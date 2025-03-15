#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

#include "event_sink.h"
#include "event_source.h"

#define RP_ERR_FAILED_TO_CREATE_SCREENSHOT_OUTPUT_DIRECTORY "Failed to create screenshot output directory"
#define RP_ERR_INVALID_SCREENSHOT_SERIALIZATION_STRATEGY "Invalid screenshot serialization strategy"

constexpr uint32_t SCREENSHOT_MIN_INTERVAL = 1;
// Tokens to identify screenshot data in the event stream
constexpr const wchar_t *SCREENSHOT_PATH_TOKEN = L"[SCREENSHOT_PATH]";
constexpr const wchar_t *SCREENSHOT_BASE64_TOKEN = L"[SCREENSHOT_BASE64]";
constexpr const wchar_t *SCREENSHOT_END_TOKEN = L"[/SCREENSHOT]";

// Forward declarations
class ScreenshotEventSource;

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

class ScreenshotEventSource : public EventSource
{
  public:
    ScreenshotEventSource();
    ~ScreenshotEventSource();

  private:
    virtual void initializeSource(std::weak_ptr<EventSink> inSink) override;
    virtual void uninitializeSource() override;

    std::weak_ptr<EventSink> outputSink;

    std::thread screenshotThread;
    // Whether or not the screenshotThread is currently capturing screenshots
    std::atomic<bool> isRunning;

    void screenshotThreadFunction();
    bool captureScreenshot();

  private:
    friend class ScreenshotEventSourceBuilder;
    // Strategy for serializing screenshots
    std::unique_ptr<ScreenshotSerializationStrategy> serializationStrategy;

    uint32_t screenshotIntervalSeconds;
    uint32_t pauseAfterIdleSeconds;
};

class ScreenshotEventSourceBuilder
{
  public:
    ScreenshotEventSourceBuilder();
    ~ScreenshotEventSourceBuilder() = default;

    ScreenshotEventSourceBuilder &withScreenshotIntervalSeconds(uint32_t screenshotIntervalSeconds);
    ScreenshotEventSourceBuilder &withPauseAfterIdleSeconds(uint32_t pauseAfterIdleSeconds);
    ScreenshotEventSourceBuilder &withScreenshotOutputDirectory(std::filesystem::path screenshotOutputDirectory);
    ScreenshotEventSourceBuilder &withScreenshotSerializationStrategy(ScreenshotSerializationStrategyType strategyType);
    std::unique_ptr<ScreenshotEventSource> build();

  private:
    void validate();

    uint32_t screenshotIntervalSeconds;
    // Number of seconds that the user can be idle before we pause taking screenshots ("Idle" meaning that the
    // user has not performed any keyboard or mouse input)
    uint32_t pauseAfterIdleSeconds;
    std::filesystem::path screenshotOutputDirectory;
    ScreenshotSerializationStrategyType serializationStrategyType;
};
