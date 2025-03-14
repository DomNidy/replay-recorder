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

// Base64 encoding implementation
std::string Base64SerializationStrategy::encodeBase64(const BYTE *data, size_t dataLength) const
{
    std::string encoded;
    encoded.reserve((dataLength + 2) / 3 * 4); // Reserve space for the base64 output

    for (size_t i = 0; i < dataLength; i += 3)
    {
        uint32_t octet_a = i < dataLength ? data[i] : 0;
        uint32_t octet_b = i + 1 < dataLength ? data[i + 1] : 0;
        uint32_t octet_c = i + 2 < dataLength ? data[i + 2] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

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

    // Convert to wide string
    std::wstring wBase64Data(base64Data.begin(), base64Data.end());

    // Write the token and base64 data to the event sink
    *sink << SCREENSHOT_BASE64_TOKEN << wBase64Data.c_str() << SCREENSHOT_END_TOKEN;

    return true;
}

ScreenshotEventSource::ScreenshotEventSource() : outputSink(nullptr), isRunning(false)
{
    ScreenshotEventSourceConfig config;
    // Default config, but validate it anyway
    config.validate();

    screenshotIntervalSeconds = config.screenshotIntervalSeconds;

    switch (config.strategyType)
    {
    case ScreenshotSerializationStrategyType::FilePath:
        serializationStrategy = std::make_unique<FilePathSerializationStrategy>(config.screenshotOutputDirectory);
        break;
    case ScreenshotSerializationStrategyType::Base64:
        serializationStrategy = std::make_unique<Base64SerializationStrategy>();
        break;
    default:
        serializationStrategy = std::make_unique<FilePathSerializationStrategy>(config.screenshotOutputDirectory);
        break;
    }
}

ScreenshotEventSource::ScreenshotEventSource(ScreenshotEventSourceConfig config) : outputSink(nullptr), isRunning(false)
{
    config.validate();
    screenshotIntervalSeconds = config.screenshotIntervalSeconds;
    switch (config.strategyType)
    {
    case ScreenshotSerializationStrategyType::FilePath:
        serializationStrategy = std::make_unique<FilePathSerializationStrategy>(config.screenshotOutputDirectory);
        break;
    case ScreenshotSerializationStrategyType::Base64:
        serializationStrategy = std::make_unique<Base64SerializationStrategy>();
        break;
    default:
        serializationStrategy = std::make_unique<FilePathSerializationStrategy>(config.screenshotOutputDirectory);
        break;
    }
}

void ScreenshotEventSource::initializeSource(EventSink *inSink)
{
    if (!inSink)
    {
        throw std::runtime_error(RP_ERR_INITIALIZED_WITH_NULLPTR_EVENT_SINK);
    }
    outputSink = inSink;

    spdlog::info("Initialized ScreenshotEventSource");
    spdlog::info("\t Screenshot Interval (secs): {}", screenshotIntervalSeconds);

    // Start SS thread
    isRunning = true;
    screenshotThread = std::thread(&ScreenshotEventSource::screenshotThreadFunction, this);
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

// Takes a screenshot then sleeps for interval seconds
void ScreenshotEventSource::screenshotThreadFunction()
{
    while (isRunning)
    {
        captureScreenshot();
        std::this_thread::sleep_for(std::chrono::seconds(screenshotIntervalSeconds));
    }
}

bool ScreenshotEventSource::captureScreenshot()
{
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create a device context (DC) for the entire screen
    HDC screenDC = GetDC(NULL);
    if (!screenDC)
    {
        spdlog::error("Failed to get screen DC");
        return false;
    }

    // Create a compatible DC in memory
    HDC memDC = CreateCompatibleDC(screenDC);
    if (!memDC)
    {
        spdlog::error("Failed to create compatible DC");
        ReleaseDC(NULL, screenDC); // Clean up screenDC
        return false;
    }
    // Create a bitmap compatible with the screen DC
    HBITMAP bitmap = CreateCompatibleBitmap(screenDC, screenWidth, screenHeight);
    if (!bitmap)
    {
        spdlog::error("Failed to create compatible bitmap");
        DeleteDC(memDC);           // Clean up memDC
        ReleaseDC(NULL, screenDC); // Clean up screenDC
        return false;
    }

    // Select the bitmap into the memory DC
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
    if (outputSink && serializationStrategy)
    {
        result = serializationStrategy->serializeScreenshot(this, outputSink, rgbData, screenWidth, screenHeight, 3);
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

void ScreenshotEventSourceConfig::validate()
{
    // Ensure output directory exists if using FilePath strategy
    if (strategyType == ScreenshotSerializationStrategyType::FilePath &&
        !std::filesystem::exists(this->screenshotOutputDirectory))
    {
        spdlog::info("The screenshot output directory '{}' did not exist. Creating it.",
                     this->screenshotOutputDirectory.string());
        bool didCreateSucceed = std::filesystem::create_directory(this->screenshotOutputDirectory);
        if (!didCreateSucceed)
        {
            spdlog::error("Failed to create screenshot output directory at '{}'!",
                          this->screenshotOutputDirectory.string());
            throw std::runtime_error("Failed to create screenshot output directory");
        }
    }

    // Ensure screenshot interval is at least 1 second
    if (this->screenshotIntervalSeconds <= SCREENSHOT_MIN_INTERVAL)
    {
        spdlog::warn("Screenshot interval of {} seconds was passed. This is invalid, so the interval has been "
                     "set to {} second.",
                     this->screenshotIntervalSeconds, SCREENSHOT_MIN_INTERVAL);
        this->screenshotIntervalSeconds = 1;
    }
}

ScreenshotEventSourceConfig::ScreenshotEventSourceConfig()
    : screenshotIntervalSeconds(5), screenshotOutputDirectory(std::filesystem::path("./replay-screenshots")),
      strategyType(ScreenshotSerializationStrategyType::FilePath)
{
    validate();
}

ScreenshotEventSourceConfig::ScreenshotEventSourceConfig(uint32_t screenshotIntervalSeconds,
                                                         std::filesystem::path screenshotOutputDirectory,
                                                         ScreenshotSerializationStrategyType strategyType)
    : screenshotIntervalSeconds(screenshotIntervalSeconds), screenshotOutputDirectory(screenshotOutputDirectory),
      strategyType(strategyType)
{
    validate();
};

ScreenshotEventSource::~ScreenshotEventSource()
{
    uninitializeSource();
}