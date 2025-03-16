#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <thirdparty/stb_image_write.h>

#include <algorithm>
#include <string>
#include <vector>
#include "event_source.h"
#include "screenshot_event_source.h"

// Base64 encoding table
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

std::string Base64SerializationStrategy::encodeBase64(const BYTE *data, size_t dataLength) const
{
    std::string encoded;
    encoded.reserve((dataLength + 2) / 3 * 4); // Reserve space for the base64 output

    // Process input data in chunks of 3 bytes
    for (size_t i = 0; i < dataLength; i += 3)
    {
        uint32_t octet_a = i < dataLength ? data[i] : 0;         // First byte
        uint32_t octet_b = i + 1 < dataLength ? data[i + 1] : 0; // Second byte
        uint32_t octet_c = i + 2 < dataLength ? data[i + 2] : 0; // Third byte

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        encoded.push_back(base64_chars[(triple >> 18) & 0x3F]);
        encoded.push_back(base64_chars[(triple >> 12) & 0x3F]);

        encoded.push_back(i + 1 < dataLength ? base64_chars[(triple >> 6) & 0x3F] : '=');
        encoded.push_back(i + 2 < dataLength ? base64_chars[triple & 0x3F] : '=');
    }

    return encoded;
}

// FilePathSerializationStrategy implementation
bool FilePathSerializationStrategy::serializeScreenshot(const ScreenshotEventSource *source, EventSink *sink,
                                                        const BYTE *imageData, int width, int height,
                                                        int channels) const
{
    if (!sink || !source)
    {
        spdlog::error("Null sink or source in FilePathSerializationStrategy");
        return false;
    }

    // Save the screenshot to a file
    std::string filePath = saveScreenshotToFile(outputDirectory, imageData, width, height, channels);
    if (filePath.empty())
    {
        spdlog::error("Failed to save screenshot to file");
        return false;
    }

    // Convert to wide string
    std::wstring wFilePath(filePath.begin(), filePath.end());

    // Write the token and file path to the event sink
    *sink << SCREENSHOT_PATH_TOKEN << wFilePath.c_str() << SCREENSHOT_END_TOKEN;

    return true;
}

// Base64SerializationStrategy implementation
bool Base64SerializationStrategy::serializeScreenshot(const ScreenshotEventSource *source, EventSink *sink,
                                                      const BYTE *imageData, int width, int height, int channels) const
{
    if (!sink || !imageData)
    {
        spdlog::error("Null sink or image data in Base64SerializationStrategy");
        return false;
    }

    // Calculate image data size
    size_t dataSize = width * height * channels;

    // Encode the image data as base64
    std::string base64Data = encodeBase64(imageData, dataSize);

    // Write the token and base64 data to the event sink
    *sink << SCREENSHOT_BASE64_TOKEN << base64Data.c_str() << SCREENSHOT_END_TOKEN;

    return true;
}

ScreenshotEventSource::ScreenshotEventSource() : outputSink(std::weak_ptr<EventSink>()), isRunning(false)
{
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
    screenshotThread = std::thread(&ScreenshotTimingStrategy::screenshotThreadFunction, timingStrategy.get(), this,
                                   &ScreenshotEventSource::captureScreenshot);

    spdlog::info("ScreenshotEventSource successfully initialized");
}

void ScreenshotEventSource::uninitializeSource()
{
    if (isRunning)
    {
        isRunning = false;
        if (screenshotThread.joinable())
        {
            screenshotThread.join();
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
    if (!outputSink.expired() && serializationStrategy)
    {
        result = serializationStrategy->serializeScreenshot(this, outputSink.lock().get(), rgbData, screenWidth,
                                                            screenHeight, 3);
    }
    else
    {
        spdlog::error("Cannot serialize screenshot: Sink or strategy is null");
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

// Helper method to save screenshot to file
std::string FilePathSerializationStrategy::saveScreenshotToFile(std::filesystem::path outputDirectory,
                                                                const BYTE *imageData, int width, int height,
                                                                int channels) const
{
    if (!imageData)
    {
        spdlog::error("Null image data in saveScreenshotToFile");
        return "";
    }

    // Get current timestamp
    std::time_t currentTime = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &currentTime); // Use localtime_s for thread safety

    // Create filename
    std::ostringstream filenameStream;
    filenameStream << outputDirectory.string() << "/ss_" << std::put_time(&localTime, "%Y-%m-%d_%H-%M-%S") << ".png";
    std::string outputFilepath = filenameStream.str();

    int result = stbi_write_png(outputFilepath.c_str(), width, height,
                                channels, // RGB components
                                imageData,
                                width * channels // stride in bytes
    );

    if (result == 0)
    {
        spdlog::error("Failed to write PNG file to {}", outputFilepath);
        return "";
    }

    spdlog::info("Screenshot saved to: {}", outputFilepath);
    return outputFilepath;
}

ScreenshotEventSource::~ScreenshotEventSource()
{
    uninitializeSource();
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
    std::unique_ptr<ScreenshotTimingStrategy> timingStrategy)
{
    this->timingStrategy = std::move(timingStrategy);
    return *this;
}

std::unique_ptr<ScreenshotEventSource> ScreenshotEventSourceBuilder::build()
{
    validate();

    std::unique_ptr<ScreenshotEventSource> source = std::make_unique<ScreenshotEventSource>();

    source->timingStrategy = std::move(timingStrategy);

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

    return source;
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

bool FixedIntervalScreenshotTimingStrategy::screenshotThreadFunction(
    ScreenshotEventSource *source, CaptureScreenshotFunction captureScreenshotFunction) const
{
    spdlog::info("Screenshot timing strategy: Fixed interval");
    while (source && source->getIsRunning())
    {

        if (!isIdle(pauseAfterIdleSeconds))
        {
            spdlog::info("Capturing screenshot");
            (source->*captureScreenshotFunction)();
        }
        else
        {
            uint32_t timeSinceLastInput = getCurrentTime() - getLastInputTime();
            spdlog::info("Skipping screenshot because no input was detected for {} seconds", timeSinceLastInput / 1000);
        }

        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
    }
    return true;
}

bool WindowChangeScreenshotTimingStrategy::screenshotThreadFunction(
    ScreenshotEventSource *source, CaptureScreenshotFunction captureScreenshotFunction) const
{
    spdlog::info("Screenshot timing strategy: Window change");
    return true;
}

uint32_t ScreenshotTimingStrategy::getLastInputTime() const
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lii);
    return lii.dwTime;
}

uint32_t ScreenshotTimingStrategy::getCurrentTime() const
{
    return GetTickCount();
}

bool ScreenshotTimingStrategy::isIdle(uint32_t idleThresholdSeconds) const
{
    uint32_t currentTime = getCurrentTime();
    uint32_t lastInputTime = getLastInputTime();
    uint32_t timeSinceLastInput;

    // Handle wrap around edge case
    if (currentTime >= lastInputTime)
    {
        timeSinceLastInput = currentTime - lastInputTime;
    }
    else
    {
        timeSinceLastInput = (MAXDWORD - lastInputTime) + currentTime + 1;
    }

    return timeSinceLastInput >= idleThresholdSeconds * 1000;
}
