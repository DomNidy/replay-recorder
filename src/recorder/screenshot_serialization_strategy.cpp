#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <thirdparty/stb_image_write.h>

#include <algorithm>
#include <string>
#include <vector>
#include "screenshot_event_source.h"
#include "screenshot_serialization_strategy.h"

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
    *sink << SCREENSHOT_PATH_TOKEN << "\"" << wFilePath.c_str() << "\"" << SCREENSHOT_END_TOKEN;

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