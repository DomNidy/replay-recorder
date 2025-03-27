#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "event_sink.h"
#include "event_source.h"
#include "screenshot_serialization_strategy.h"
#include "screenshot_timing_strategy.h"

#define RP_ERR_FAILED_TO_CREATE_SCREENSHOT_OUTPUT_DIRECTORY "Failed to create screenshot output directory"
#define RP_ERR_INVALID_SCREENSHOT_SERIALIZATION_STRATEGY "Invalid screenshot serialization strategy"

// ScreenshotEventSource captures screenshots of the user's focused monitor
// and sends them to an EventSink.
//
// Key characteristics:
// - Uses Windows API to capture the contents of the focused monitor
// - Follows the Strategy pattern for flexible configuration:
//   1. ScreenshotTimingStrategy: Controls when to take screenshots (on window changes or at fixed intervals)
//   2. ScreenshotSerializationStrategy: Controls how to serialize screenshots (as file paths or base64 data)
// - Runs in a dedicated background thread to avoid blocking the main application
// - Includes idle detection to avoid taking screenshots when the user is inactive
// - Can be configured via the ScreenshotEventSourceBuilder class
//
// Usage:
// The class is intended to be created using the ScreenshotEventSourceBuilder, which
// provides a fluent interface for configuration. Example:
//
//
// ```cpp
// auto builder = ScreenshotEventSourceBuilder()
//     .withScreenshotOutputDirectory("./screenshots")
//     .withScreenshotSerializationStrategy(ScreenshotSerializationStrategyType::FilePath)
//     .withScreenshotTimingStrategy(std::make_shared<WindowChangeScreenshotTimingStrategy>());
// auto source = builder.build();
// ```
class ScreenshotEventSource : public EventSource
{
    // These classes need access to the captureScreenshot method
    friend class WindowChangeScreenshotTimingStrategy;
    friend class FixedIntervalScreenshotTimingStrategy;

  public:
    ScreenshotEventSource();
    ~ScreenshotEventSource();

    // Returns whether the screenshot thread is currently running
    bool getIsRunning() const;

  public:
    //~ Begin EventSource interface
    virtual void initializeSource(std::shared_ptr<EventSink> inSink) override;
    //~ End EventSource interface

  private:
    // Reference to the event sink where screenshots will be sent
    std::shared_ptr<EventSink> outputSink;

    // Whether or not the screenshotThread is currently capturing screenshots
    std::atomic<bool> isRunning;

    // Captures a screenshot of the focused monitor and serializes it
    // using the configured serialization strategy
    bool captureScreenshot();

    // Returns the monitor info of the focused monitor
    // Returns std::nullopt if no monitor could be identified
    std::optional<MONITORINFO> getFocusedMonitorInfo();

  private:
    friend class ScreenshotEventSourceBuilder;

    // The thread that is capturing screenshots
    std::thread screenshotThread;

    // Strategy for serializing screenshots
    std::unique_ptr<ScreenshotSerializationStrategy> serializationStrategy;

    // Strategy for deciding when to take screenshots
    std::shared_ptr<ScreenshotTimingStrategy> timingStrategy;
};

// ScreenshotEventSourceBuilder provides a fluent interface for creating properly configured
// ScreenshotEventSource instances. It follows the Builder pattern to simplify the creation of
// ScreenshotEventSource objects.
//
// The builder handles:
// - Setting up the screenshot output directory (creating it if necessary)
// - Configuring the serialization strategy (file path or base64)
// - Configuring the timing strategy (window change or fixed interval)
// - Validating the configuration before building
//
// Example Usage:
// ```cpp
// auto builder = ScreenshotEventSourceBuilder()
//     .withScreenshotOutputDirectory("./screenshots")
//     .withScreenshotSerializationStrategy(ScreenshotSerializationStrategyType::FilePath)
//     .withScreenshotTimingStrategy(std::make_shared<WindowChangeScreenshotTimingStrategy>());
// auto source = builder.build();
// ```
class ScreenshotEventSourceBuilder
{
  public:
    ScreenshotEventSourceBuilder();
    ~ScreenshotEventSourceBuilder()
    {
        LOG_CLASS_DEBUG("ScreenshotEventSourceBuilder", "Destructor called");
    }

    // Sets the directory where screenshots will be saved when using FilePath serialization strategy
    // The directory will be created if it doesn't exist during build()
    ScreenshotEventSourceBuilder& withScreenshotOutputDirectory(std::filesystem::path screenshotOutputDirectory);

    // Sets the strategy type for serializing screenshots:
    // - FilePath: Save screenshots to files. File paths are sent to the event sink
    // - Base64: Encode screenshots as base64. Base64 data is sent to the event sink
    ScreenshotEventSourceBuilder& withScreenshotSerializationStrategy(
        ScreenshotSerializationStrategyType serializationStrategyType);

    // Sets the strategy for determining when to capture screenshots:
    // - WindowChangeScreenshotTimingStrategy: Take screenshots when window focus changes
    // - FixedIntervalScreenshotTimingStrategy: Take screenshots at fixed time intervals
    ScreenshotEventSourceBuilder& withScreenshotTimingStrategy(
        std::shared_ptr<ScreenshotTimingStrategy> timingStrategy);
    std::shared_ptr<ScreenshotEventSource> build();

  private:
    void validate();

    // The directory to save screenshots to (only used if serialization strategy is FilePath)
    std::filesystem::path screenshotOutputDirectory;

    // Enum type for the strategy to use when serializing screenshots
    ScreenshotSerializationStrategyType serializationStrategyType;

    // ScreenshotTimingStrategy instance to use, this determines when screenshots are taken
    std::shared_ptr<ScreenshotTimingStrategy> timingStrategy;
};
