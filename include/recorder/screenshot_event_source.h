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

enum class ScreenshotTimingStrategyType
{
    WindowChange,
    FixedInterval
};

typedef bool (ScreenshotEventSource::*CaptureScreenshotFunction)();

class ScreenshotTimingStrategy
{
  public:
    virtual ~ScreenshotTimingStrategy() = default;
    ScreenshotTimingStrategy() = default;

    virtual bool screenshotThreadFunction(ScreenshotEventSource *source,
                                          CaptureScreenshotFunction captureScreenshotFunction) const = 0;

  protected:
    // Get the time of the last input event
    virtual bool isIdle(uint32_t idleThresholdSeconds) const;
    virtual uint32_t getLastInputTime() const;
    virtual uint32_t getCurrentTime() const;
};

// Take a screenshot whenever the user's window changes
class WindowChangeScreenshotTimingStrategy : public ScreenshotTimingStrategy
{
  public:
    virtual bool screenshotThreadFunction(ScreenshotEventSource *source,
                                          CaptureScreenshotFunction captureScreenshotFunction) const override;
};

// Take a screenshot every N seconds
class FixedIntervalScreenshotTimingStrategy : public ScreenshotTimingStrategy
{
  public:
    FixedIntervalScreenshotTimingStrategy(uint32_t intervalSeconds, uint32_t pauseAfterIdleSeconds)
        : intervalSeconds(intervalSeconds), pauseAfterIdleSeconds(pauseAfterIdleSeconds)
    {
    }

    virtual bool screenshotThreadFunction(ScreenshotEventSource *source,
                                          CaptureScreenshotFunction captureScreenshotFunction) const override;

  private:
    // Number of seconds between screenshots
    uint32_t intervalSeconds;

    // Number of seconds that the user can be idle before we pause taking screenshots ("Idle" meaning that the
    // user has not performed any keyboard or mouse input)
    uint32_t pauseAfterIdleSeconds;
};

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
    // Make these classes friends so they can call captureScreenshot()
    friend class FixedIntervalScreenshotTimingStrategy;
    friend class WindowChangeScreenshotTimingStrategy;

  public:
    ScreenshotEventSource();
    ~ScreenshotEventSource();

    bool getIsRunning() const;

  private:
    virtual void initializeSource(std::weak_ptr<EventSink> inSink) override;
    virtual void uninitializeSource() override;

    std::weak_ptr<EventSink> outputSink;

    // Whether or not the screenshotThread is currently capturing screenshots
    std::atomic<bool> isRunning;

    bool captureScreenshot();

  private:
    friend class ScreenshotEventSourceBuilder;

    // The thread that is capturing screenshots
    std::thread screenshotThread;

    // Strategy for serializing screenshots
    std::unique_ptr<ScreenshotSerializationStrategy> serializationStrategy;

    // Strategy for deciding when to take screenshots
    std::unique_ptr<ScreenshotTimingStrategy> timingStrategy;
};

class ScreenshotEventSourceBuilder
{
  public:
    ScreenshotEventSourceBuilder();
    ~ScreenshotEventSourceBuilder() = default;

    ScreenshotEventSourceBuilder &withScreenshotOutputDirectory(std::filesystem::path screenshotOutputDirectory);
    ScreenshotEventSourceBuilder &withScreenshotSerializationStrategy(
        ScreenshotSerializationStrategyType serializationStrategyType);
    ScreenshotEventSourceBuilder &withScreenshotTimingStrategy(
        std::unique_ptr<ScreenshotTimingStrategy> timingStrategy);
    std::unique_ptr<ScreenshotEventSource> build();

  private:
    void validate();

    // The directory to save screenshots to (only used if serialization strategy is FilePath)
    std::filesystem::path screenshotOutputDirectory;

    // Enum type for the strategy to use when serializing screenshots
    ScreenshotSerializationStrategyType serializationStrategyType;

    // ScreenshotTimingStrategy instance to use, this determines when screenshots are taken
    std::unique_ptr<ScreenshotTimingStrategy> timingStrategy;
};
