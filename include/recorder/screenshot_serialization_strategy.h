#pragma once
#include <windows.h>
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

#include "event_sink.h"

// Forward declarations
class ScreenshotEventSource;

// Tokens to identify screenshot data in the event stream
constexpr const wchar_t *SCREENSHOT_PATH_TOKEN = L"[SCREENSHOT_PATH]";
constexpr const wchar_t *SCREENSHOT_BASE64_TOKEN = L"[SCREENSHOT_BASE64]";
constexpr const wchar_t *SCREENSHOT_END_TOKEN = L"[/SCREENSHOT]";

// Specifies the strategy for serializing screenshots
// FilePath: Save the screenshot to a file and send the file path to the event sink.
// Base64: Encode the screenshot as base64 and send it to the event sink.
// With both strategies, the serialized screenshot is embedded in the activity stream as a
// special token, but the serialized format differs.
enum class ScreenshotSerializationStrategyType
{
    FilePath,
    Base64
};

// Base class for screenshot serialization strategies
class ScreenshotSerializationStrategy
{
  public:
    ScreenshotSerializationStrategy() = default;
    virtual ~ScreenshotSerializationStrategy() = default;

    // Serialize the screenshot and send it to the event sink
    virtual bool serializeScreenshot(const ScreenshotEventSource *source, EventSink *sink, const BYTE *imageData,
                                     int width, int height, int channels) const = 0;
};

// Strategy to save screenshot to file and send the file path to the event sink
class FilePathSerializationStrategy : public ScreenshotSerializationStrategy
{
  public:
    FilePathSerializationStrategy(std::filesystem::path outputDirectory) : outputDirectory(outputDirectory)
    {
    }

    ~FilePathSerializationStrategy()
    {
        spdlog::debug("FilePathSerializationStrategy::~FilePathSerializationStrategy: Destructor called");
    }

    virtual bool serializeScreenshot(const ScreenshotEventSource *source, EventSink *sink, const BYTE *imageData,
                                     int width, int height, int channels) const override;

    std::string saveScreenshotToFile(std::filesystem::path outputDirectory, const BYTE *imageData, int width,
                                     int height, int channels) const;

  private:
    std::filesystem::path outputDirectory;
};

// Strategy to encode screenshot as base64 and send it directly to the event sink
class Base64SerializationStrategy : public ScreenshotSerializationStrategy
{
  public:
    virtual bool serializeScreenshot(const ScreenshotEventSource *source, EventSink *sink, const BYTE *imageData,
                                     int width, int height, int channels) const override;

  private:
    // Helper method to encode binary data to base64
    std::string encodeBase64(const BYTE *data, size_t dataLength) const;
};