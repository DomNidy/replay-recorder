#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

#include "event_sink.h"
#include "event_source.h"
#include "screenshot_serialization_strategy.h"
#include "screenshot_timing_strategy.h"

#define RP_ERR_FAILED_TO_CREATE_SCREENSHOT_OUTPUT_DIRECTORY "Failed to create screenshot output directory"
#define RP_ERR_INVALID_SCREENSHOT_SERIALIZATION_STRATEGY "Invalid screenshot serialization strategy"

class ScreenshotEventSource : public EventSource
{
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
