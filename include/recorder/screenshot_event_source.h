#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

#include "event_sink.h"
#include "event_source.h"

constexpr uint32_t SCREENSHOT_MIN_INTERVAL = 1;

class ScreenshotEventSourceConfig
{

  public:
    ScreenshotEventSourceConfig();
    ScreenshotEventSourceConfig(uint32_t screenshotIntervalSeconds, std::filesystem::path screenshotOutputDirectory);

  private:
    friend class ScreenshotEventSource;
    uint32_t screenshotIntervalSeconds;
    std::filesystem::path screenshotOutputDirectory;

    // Ensures that the output directory and screenshot interval are valid
    void validate();
};

class ScreenshotEventSource : public EventSource
{

  public:
    ScreenshotEventSource();
    ScreenshotEventSource(ScreenshotEventSourceConfig config);
    
    // Returns the instance with the specified configuration.
    void setConfig(ScreenshotEventSourceConfig config);

  private:
    ~ScreenshotEventSource()
    {
        uninitializeSource();
    }

  private:
    virtual void initializeSource(EventSink *inSink) override;
    virtual void uninitializeSource() override;

  private:
    EventSink *outputSink;

    // Configures screenshot-related settings and behavior (like interval and output dir)
    ScreenshotEventSourceConfig config;

    std::thread screenshotThread;
    // Whether or not the screenshotThread is currently capturing screenshots
    std::atomic<bool> isRunning;

  private:
    void screenshotThreadFunction();
    inline bool captureScreenshot();
};