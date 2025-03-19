#include "screenshot_event_source.h"
#include <algorithm>
#include <string>
#include <vector>
#include "event_source.h"
#include "screenshot_serialization_strategy.h"
#include "screenshot_timing_strategy.h"

ScreenshotEventSource::ScreenshotEventSource() : outputSink(std::weak_ptr<EventSink>()), isRunning(false)
{
}

ScreenshotEventSource::~ScreenshotEventSource()
{
    spdlog::debug("ScreenshotEventSource::~ScreenshotEventSource: Destructor called");
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

    spdlog::info("ScreenshotEventSource successfully initialized");
}

void ScreenshotEventSource::uninitializeSource()
{
    spdlog::debug("Uninitializing ScreenshotEventSource in thread {}", GetCurrentThreadId());
    if (isRunning)
    {
        isRunning = false;
        if (screenshotThread.joinable())
        {
            spdlog::debug("ScreenshotEventSource: Screenshot thread is joinable, joining it");
            screenshotThread.join();
        }
        else
        {
            spdlog::debug("ScreenshotEventSource: Screenshot thread is not joinable, it is already joined");
        }
        spdlog::info("ScreenshotEventSource uninitialized");
    }
}

bool ScreenshotEventSource::getIsRunning() const
{
    return isRunning;
}

bool ScreenshotEventSource::captureScreenshot()
{
    // Lock sink so we can use it to prevent it from getting destroyed while we're using it
    auto lockedSink = outputSink.lock();
    if (!lockedSink)
    {
        spdlog::warn(
            "ScreenshotEventSource::captureScreenshot: captureScreenshot ran, but the output EventSink was already "
            "expired. We "
            "won't be able to serialize this screenshot. Warning so I remember to fix this (and make it better)");
        return false;
    }

    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create a device context (DC) for the entire screen
    // Device contexts sort of act like fd's and we use them
    // to access "read/write" capabilities of graphics devices
    HDC screenDC = GetDC(NULL);
    if (!screenDC)
    {
        spdlog::error("Failed to get screen DC");
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
        spdlog::error("Failed to create compatible DC");
        ReleaseDC(NULL, screenDC); // Clean up screenDC
        return false;
    }

    // Create a bitmap compatible with the screen DC
    // "Compatibility" is important because different screens might have
    // different color depths or different internal representations of color
    // The color format of the bitmap created by the CreateCompatibleBitmap function matches the color format of the
    // device identified by the hdc parameter. This bitmap can be selected into any memory device context that is
    // compatible with the original device.
    HBITMAP bitmap = CreateCompatibleBitmap(screenDC, screenWidth, screenHeight);
    if (!bitmap)
    {
        spdlog::error("Failed to create compatible bitmap");
        DeleteDC(memDC);           // Clean up memDC
        ReleaseDC(NULL, screenDC); // Clean up screenDC
        return false;
    }

    // Select the bitmap into the memory DC.
    // Essentially, we're telling the memory DC that all subsequent drawing operations should operate on the bitmap that
    // we pass it. Sort of similar to how OpenGL is a state machine.
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, bitmap);
    if (!oldBitmap)
    {
        spdlog::error("Failed to select object into DC");
        DeleteObject(bitmap);      // Clean up bitmap
        DeleteDC(memDC);           // Clean up memDC
        ReleaseDC(NULL, screenDC); // Clean up screenDC
        return false;
    }

    // BitBlt the screen content into the memory DC's selected bitmap
    if (!BitBlt(memDC, 0, 0, screenWidth, screenHeight, screenDC, 0, 0, SRCCOPY))
    {
        spdlog::error("Failed to BitBlt: {}", GetLastError());
        SelectObject(memDC, oldBitmap); // Restore old bitmap
        DeleteObject(bitmap);           // Clean up bitmap
        DeleteDC(memDC);                // Clean up memDC
        ReleaseDC(NULL, screenDC);      // Clean up screenDC
        return false;
    }

    // Get bitmap data
    BITMAPINFOHEADER bi;
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = screenWidth;
    bi.biHeight = -screenHeight; // Negative height = top-down DIB
    bi.biPlanes = 1;
    bi.biBitCount = 24; // 24 bits per pixel (RGB)
    bi.biCompression = BI_RGB;

    // Allocate memory for bitmap bits
    BYTE *pixels = new (std::nothrow) BYTE[screenWidth * screenHeight * 3]; // 3 bytes per pixel (RGB)
    if (!pixels)
    {
        spdlog::error("Failed to allocate memory for pixels");
        SelectObject(memDC, oldBitmap); // Restore old bitmap
        DeleteObject(bitmap);           // Clean up bitmap
        DeleteDC(memDC);                // Clean up memDC
        ReleaseDC(NULL, screenDC);      // Clean up screenDC
        return false;
    }

    // Get the bitmap bits
    if (!GetDIBits(memDC, bitmap, 0, screenHeight, pixels, (BITMAPINFO *)&bi, DIB_RGB_COLORS))
    {
        spdlog::error("Failed to get DIBits");
        delete[] pixels;                // Clean up pixels
        SelectObject(memDC, oldBitmap); // Restore old bitmap
        DeleteObject(bitmap);           // Clean up bitmap
        DeleteDC(memDC);                // Clean up memDC
        ReleaseDC(NULL, screenDC);      // Clean up screenDC
        return false;
    }

    // Create a buffer to hold the RGB data (stbi_write_png expects RGB format)
    BYTE *rgbData = new (std::nothrow) BYTE[screenWidth * screenHeight * 3];
    if (!rgbData)
    {
        spdlog::error("Failed to allocate memory for rgbData");
        delete[] pixels;                // Clean up pixels
        SelectObject(memDC, oldBitmap); // Restore old bitmap
        DeleteObject(bitmap);           // Clean up bitmap
        DeleteDC(memDC);                // Clean up memDC
        ReleaseDC(NULL, screenDC);      // Clean up screenDC
        return false;
    }

    // Convert BGR to RGB
    for (int i = 0; i < screenWidth * screenHeight; ++i)
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
            serializationStrategy->serializeScreenshot(this, lockedSink.get(), rgbData, screenWidth, screenHeight, 3);
    }
    else
    {
        spdlog::error("Cannot serialize screenshot: SerializationStrategy is null");
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
        spdlog::debug("ScreenshotEventSourceBuilder::build: Setting timing strategy to window change");
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
            spdlog::error("Failed to create screenshot output directory: {}", screenshotOutputDirectory.string());
            throw std::runtime_error(RP_ERR_FAILED_TO_CREATE_SCREENSHOT_OUTPUT_DIRECTORY);
        }
    }
}
