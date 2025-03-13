#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <thirdparty/stb_image_write.h>

#include "event_source.h"
#include "screenshot_event_source.h"

EventSink *ScreenshotEventSource::outputSink = nullptr;

void ScreenshotEventSource::setConfig(ScreenshotEventSourceConfig config)
{
    config.validate();
    this->config = config;
}

void ScreenshotEventSource::initializeSource(EventSink *inSink)
{
    if (!inSink)
    {
        throw std::runtime_error(RP_ERR_INITIALIZED_WITH_NULLPTR_EVENT_SINK);
    }
    outputSink = inSink;
    config.validate();

    spdlog::info("Initialized ScreenshotEventSource with:");
    spdlog::info("\t Screenshot Interval (secs): {}", config.screenshotIntervalSeconds);
    spdlog::info("\t Screenshot Output Directory: {}", config.screenshotOutputDirectory.string());
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

        spdlog::info("Took SS");
        captureScreenshot();
        std::this_thread::sleep_for(std::chrono::seconds(config.screenshotIntervalSeconds));
    }
}

std::string generateScreenshotFilename(const std::filesystem::path &outputDirectory)
{
    std::time_t now = std::time(nullptr);
    char timestampPart[20];
    std::strftime(timestampPart, sizeof(timestampPart), "%H-%M-%S", std::localtime(&now));

    std::ostringstream filenameStream;
    filenameStream << "ss_" << timestampPart << ".png";
    std::string outputFilepath = (outputDirectory / filenameStream.str()).string();

    return (outputDirectory / filenameStream.str()).string();
}

bool ScreenshotEventSource::captureScreenshot()
{
    std::string outputFilepath = generateScreenshotFilename(config.screenshotOutputDirectory);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create device contexts

    // Create DC for the entire screen
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdc
    HDC screenDC = GetDC(NULL);

    // Create a compatible memory DC
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatibledc
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP bitmap = CreateCompatibleBitmap(screenDC, screenWidth, screenHeight);

    // Bitmaps can only be selected into memory DC's. A single bitmap cannot be selected into more than one DC at the
    // same time: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectobject
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, bitmap);

    // Copy data of screen DC into to memory DC (bit-block transfer)
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt
    if (!BitBlt(memDC, 0, 0, screenWidth, screenHeight, screenDC, 0, 0, SRCCOPY))
    {
        spdlog::error("BitBlt failed: {}", GetLastError());
        SelectObject(memDC, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);
        return false;
    }

    // Prepare bitmap info structure
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = screenWidth;
    bi.biHeight = -screenHeight; // Negative height means top-down DIB
    bi.biPlanes = 1;
    bi.biBitCount = 24; // 24 bits per pixel (RGB)
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // Calculate stride (bytes per row, padded to 4-byte boundary)
    int stride = ((screenWidth * 3 + 3) / 4) * 4;

    // Allocate memory for the pixel data
    BYTE *pixels = new BYTE[stride * screenHeight];

    // Copy pixel data from mem DC and write it to `pixels`
    // The bitmap header configures "how" and "what" data is written (kinda)
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-getdibits
    if (!GetDIBits(memDC, bitmap, 0, screenHeight, pixels, (BITMAPINFO *)&bi, DIB_RGB_COLORS))
    {
        spdlog::error("GetDIBits failed: {}", GetLastError());
        delete[] pixels;
        SelectObject(memDC, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);
        return false;
    }

    // Convert BGR to RGB and remove padding (stb expects RGB without padding)
    unsigned char *rgbData = new unsigned char[screenWidth * screenHeight * 3];
    for (int y = 0; y < screenHeight; y++)
    {
        for (int x = 0; x < screenWidth; x++)
        {
            int srcIdx = y * stride + x * 3;
            int dstIdx = (y * screenWidth + x) * 3;

            // BGR to RGB conversion
            rgbData[dstIdx] = pixels[srcIdx + 2];     // R
            rgbData[dstIdx + 1] = pixels[srcIdx + 1]; // G
            rgbData[dstIdx + 2] = pixels[srcIdx];     // B
        }
    }

    // Write the image as PNG using stb_image_write
    int result = stbi_write_png(outputFilepath.c_str(), screenWidth, screenHeight,
                                3, // RGB components
                                rgbData,
                                screenWidth * 3 // stride in bytes
    );

    // Clean up
    delete[] rgbData;
    delete[] pixels;
    SelectObject(memDC, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memDC);
    ReleaseDC(NULL, screenDC);

    if (result == 0)
    {
        spdlog::error("Failed to write PNG file to {}", outputFilepath);
        return false;
    }

    spdlog::info("Screenshot saved to: {}", outputFilepath);
    return true;
}

void ScreenshotEventSourceConfig::validate()
{
    // Ensure output directory exists
    if (!std::filesystem::exists(this->screenshotOutputDirectory))
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
    : screenshotIntervalSeconds(5), screenshotOutputDirectory(std::filesystem::path("./replay-screenshots"))
{
    validate();
}

ScreenshotEventSourceConfig::ScreenshotEventSourceConfig(uint32_t screenshotIntervalSeconds,
                                                         std::filesystem::path screenshotOutputDirectory)
    : screenshotIntervalSeconds(screenshotIntervalSeconds), screenshotOutputDirectory(screenshotOutputDirectory)
{
    validate();
};