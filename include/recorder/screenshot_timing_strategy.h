#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdint.h>
#include <thread>

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