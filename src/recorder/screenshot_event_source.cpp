#include "screenshot_event_source.h"
#include <algorithm>
#include <string>
#include <vector>
#include "event_source.h"
#include "screenshot_serialization_strategy.h"
#include "screenshot_timing_strategy.h"
#include "utils/logging.h"

ScreenshotEventSource::ScreenshotEventSource() : outputSink(std::weak_ptr<EventSink>()), isRunning(false)
{
}

ScreenshotEventSource::~ScreenshotEventSource()
{
    LOG_CLASS_DEBUG("ScreenshotEventSource", "Destructor called");
    // uninitializeSource();
}

void ScreenshotEventSource::initializeSource(std::weak_ptr<EventSink> inSink)
{
    assert(timingStrategy &&
           "Screenshot timing strategy not set! This should never happen because the ScreenshotEventSourceBuilder "
           "should always set it up (and we should only create ScreenshotEventSource with the builder)");

    if (inSink.expired())
    {
        throw std::runtime_error(RP_ERR_INITIALIZED_WITH_NULLPTR_EVENT_SINK);
    }
    outputSink = inSink;

    // Start SS thread
    isRunning = true;

    // Start up the screenshot thread and pass it a pointer to the captureScreenshot method, binding it to this
    // ScreenshotEventSource
    screenshotThread = std::thread(&ScreenshotTimingStrategy::screenshotThreadFunction, timingStrategy.get(), this);

    LOG_CLASS_INFO("ScreenshotEventSource", "Successfully initialized");
}

void ScreenshotEventSource::uninitializeSource()
{
    LOG_CLASS_DEBUG("ScreenshotEventSource", "Uninitializing in thread {}", GetCurrentThreadId());
    if (isRunning)
    {
        isRunning = false;
        if (screenshotThread.joinable())
        {
            LOG_CLASS_DEBUG("ScreenshotEventSource", "Screenshot thread is joinable, joining it");
            screenshotThread.join();
        }
        else
        {
            LOG_CLASS_DEBUG("ScreenshotEventSource", "Screenshot thread is not joinable, it is already joined");
        }
        LOG_CLASS_INFO("ScreenshotEventSource", "Uninitialized");
    }
}

bool ScreenshotEventSource::getIsRunning() const
{
    return isRunning;
}

std::optional<MONITORINFO> ScreenshotEventSource::getFocusedMonitorInfo()
{
    HWND foregroundWindow = GetForegroundWindow();
    if (!foregroundWindow)
    {
        LOG_CLASS_WARN("ScreenshotEventSource", "GetForegroundWindow() returned null. This can "
                                                "happen when a window loses activation.");
        return std::nullopt;
    }

    HMONITOR monitor = MonitorFromWindow(foregroundWindow, MONITOR_DEFAULTTONEAREST);

    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (!GetMonitorInfo(monitor, &monitorInfo))
    {
        LOG_CLASS_WARN("ScreenshotEventSource", "Failed to get monitor info with GetMonitorInfo()");
        return std::nullopt;
    }

    return monitorInfo;
}

bool ScreenshotEventSource::captureScreenshot()
{
    // Lock sink so we can use it to prevent it from getting destroyed while we're using it
    auto lockedSink = outputSink.lock();
    if (!lockedSink)
    {
        LOG_CLASS_WARN(
            "ScreenshotEventSource",
            "captureScreenshot ran, but the output EventSink was already "
            "expired. We "
            "won't be able to serialize this screenshot. Warning so I remember to fix this (and make it better)");
        return false;
    }

    // Try to get the monitor the focused window is in
    std::optional<MONITORINFO> _monitorInfo = getFocusedMonitorInfo();
    if (!_monitorInfo.has_value())
    {
        LOG_CLASS_WARN("ScreenshotEventSource", "getFocusedMonitorInfo() failed to get the monitor that "
                                                "the focused window is in. Skipping screenshot.");
        return false;
    }

    MONITORINFO monitorInfo = _monitorInfo.value();

    // Get the monitor dimensions
    int monitorX = monitorInfo.rcMonitor.left;
    int monitorY = monitorInfo.rcMonitor.top;
    int monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
    int monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

    LOG_CLASS_DEBUG("ScreenshotEventSource", "Capturing monitor at ({}, {}) with dimensions {}x{}", monitorX, monitorY,
                    monitorWidth, monitorHeight);

    // Create a device context (DC) for the entire screen
    // Device contexts sort of act like fd's and we use them
    // to access "read/write" capabilities of graphics devices
    HDC screenDC = GetDC(NULL);
    if (!screenDC)
    {
        LOG_CLASS_ERROR("ScreenshotEventSource", "Failed to get screen DC");
        return false;
    }

    // Create a memory DC that is compatible with the screen DC.
    // Compatibility here means that the memory DC will match
    // width and height of the device in pixels, etc.
    // "When the memory DC is created, its display surface is
    // "exactly one monochrome pixel wide and one monochrome pixel high"
    HDC memDC = CreateCompatibleDC(screenDC);
    if (!memDC)
    {
        LOG_CLASS_ERROR("ScreenshotEventSource", "Failed to create compatible DC");
        ReleaseDC(NULL, screenDC); // Clean up screenDC
        return false;
    }

    // Create a bitmap compatible with the screen DC
    HBITMAP bitmap = CreateCompatibleBitmap(screenDC, monitorWidth, monitorHeight);
    if (!bitmap)
    {
        LOG_CLASS_ERROR("ScreenshotEventSource", "Failed to create compatible bitmap");
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);
        return false;
    }

    // Select the bitmap into the memory DC
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, bitmap);
    if (!oldBitmap)
    {
        LOG_CLASS_ERROR("ScreenshotEventSource", "Failed to select object into DC");
        DeleteObject(bitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);
        return false;
    }

    // BitBlt the specific monitor content into the memory DC's selected bitmap
    if (!BitBlt(memDC, 0, 0, monitorWidth, monitorHeight, screenDC, monitorX, monitorY, SRCCOPY))
    {
        LOG_CLASS_ERROR("ScreenshotEventSource", "Failed to BitBlt: {}", GetLastError());
        SelectObject(memDC, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);
        return false;
    }

    // Get bitmap data
    BITMAPINFOHEADER bi;
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = monitorWidth;
    bi.biHeight = -monitorHeight; // Negative height = top-down DIB
    bi.biPlanes = 1;
    bi.biBitCount = 24; // 24 bits per pixel (RGB)
    bi.biCompression = BI_RGB;

    // Allocate memory for bitmap bits
    BYTE *pixels = new (std::nothrow) BYTE[monitorWidth * monitorHeight * 3]; // 3 bytes per pixel (RGB)
    if (!pixels)
    {
        LOG_CLASS_ERROR("ScreenshotEventSource", "Failed to allocate memory for pixels");
        SelectObject(memDC, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);
        return false;
    }

    // Get the bitmap bits
    if (!GetDIBits(memDC, bitmap, 0, monitorHeight, pixels, (BITMAPINFO *)&bi, DIB_RGB_COLORS))
    {
        LOG_CLASS_ERROR("ScreenshotEventSource", "Failed to get DIBits");
        delete[] pixels;
        SelectObject(memDC, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);
        return false;
    }

    // Create a buffer to hold the RGB data (stbi_write_png expects RGB format)
    BYTE *rgbData = new (std::nothrow) BYTE[monitorWidth * monitorHeight * 3];
    if (!rgbData)
    {
        LOG_CLASS_ERROR("ScreenshotEventSource", "Failed to allocate memory for rgbData");
        delete[] pixels;
        SelectObject(memDC, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);
        return false;
    }

    // Convert BGR to RGB
    for (int i = 0; i < monitorWidth * monitorHeight; ++i)
    {
        rgbData[i * 3] = pixels[i * 3 + 2];     // Red
        rgbData[i * 3 + 1] = pixels[i * 3 + 1]; // Green
        rgbData[i * 3 + 2] = pixels[i * 3];     // Blue
    }

    bool result = false;
    // Use the serialization strategy to process the screenshot
    if (serializationStrategy)
    {
        result =
            serializationStrategy->serializeScreenshot(this, lockedSink.get(), rgbData, monitorWidth, monitorHeight, 3);
    }
    else
    {
        LOG_CLASS_ERROR("ScreenshotEventSource", "Cannot serialize screenshot: SerializationStrategy is null");
    }

    // Clean up
    delete[] rgbData;
    delete[] pixels;
    SelectObject(memDC, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);

    return result;
}

ScreenshotEventSourceBuilder::ScreenshotEventSourceBuilder()
{
    // Defaults for the builder
    screenshotOutputDirectory = std::filesystem::path("./replay-screenshots");
    serializationStrategyType = ScreenshotSerializationStrategyType::FilePath;
    timingStrategy = std::make_unique<FixedIntervalScreenshotTimingStrategy>(60, 60);
}

ScreenshotEventSourceBuilder &ScreenshotEventSourceBuilder::withScreenshotOutputDirectory(
    std::filesystem::path screenshotOutputDirectory)
{
    this->screenshotOutputDirectory = screenshotOutputDirectory;
    return *this;
}

ScreenshotEventSourceBuilder &ScreenshotEventSourceBuilder::withScreenshotSerializationStrategy(
    ScreenshotSerializationStrategyType strategyType)
{
    this->serializationStrategyType = strategyType;
    return *this;
}

ScreenshotEventSourceBuilder &ScreenshotEventSourceBuilder::withScreenshotTimingStrategy(
    std::shared_ptr<ScreenshotTimingStrategy> timingStrategy)
{
    this->timingStrategy = timingStrategy;
    return *this;
}

std::unique_ptr<ScreenshotEventSource> ScreenshotEventSourceBuilder::build()
{
    validate();

    std::unique_ptr<ScreenshotEventSource> source = std::make_unique<ScreenshotEventSource>();

    // Give source a pointer to its timing strategy to use
    source->timingStrategy = timingStrategy;

    if (auto windowChangeTimingStrategy =
            dynamic_cast<WindowChangeScreenshotTimingStrategy *>(source->timingStrategy.get()))
    {
        LOG_CLASS_DEBUG("ScreenshotEventSourceBuilder", "Setting timing strategy to window change");
        windowChangeTimingStrategy->source = source.get();
    }

    switch (serializationStrategyType)
    {
    case ScreenshotSerializationStrategyType::FilePath:
        source->serializationStrategy = std::make_unique<FilePathSerializationStrategy>(screenshotOutputDirectory);
        break;
    case ScreenshotSerializationStrategyType::Base64:
        source->serializationStrategy = std::make_unique<Base64SerializationStrategy>();
        break;
    default:
        throw std::runtime_error(RP_ERR_INVALID_SCREENSHOT_SERIALIZATION_STRATEGY);
    }

    return std::move(source);
}

void ScreenshotEventSourceBuilder::validate()
{
    if (serializationStrategyType == ScreenshotSerializationStrategyType::FilePath &&
        !std::filesystem::exists(screenshotOutputDirectory))
    {
        if (!std::filesystem::create_directories(screenshotOutputDirectory))
        {
            LOG_CLASS_ERROR("ScreenshotEventSourceBuilder", "Failed to create screenshot output directory: {}",
                            screenshotOutputDirectory.string());
            throw std::runtime_error(RP_ERR_FAILED_TO_CREATE_SCREENSHOT_OUTPUT_DIRECTORY);
        }
    }
}
