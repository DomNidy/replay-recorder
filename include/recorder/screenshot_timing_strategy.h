#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdint.h>
#include <thread>
#include "windows_hook_manager.h"

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

    virtual bool screenshotThreadFunction(ScreenshotEventSource *source) = 0;

  protected:
    // Get the time of the last input event
    virtual bool isIdle(uint32_t idleThresholdSeconds) const;
    virtual uint32_t getLastInputTime() const;
    virtual uint32_t getCurrentTime() const;
};

// WindowChangeScreenshotTimingStrategy implements a hybrid screenshot timing approach:
//
// 1. Window Change Detection: Takes screenshots when the user switches between windows
//    - Implements debouncing to prevent excessive screenshots during rapid window switching
//    - Only triggers on actual window focus changes, not other window events
//
// 2. Fixed Interval Backup: Additionally takes screenshots at regular intervals
//    - Ensures activity is captured even when the user stays in one window for extended periods
//    - Interval screenshots continue regardless of window change debouncing
//
// 3. Idle Detection: Pauses all screenshot capture during periods of user inactivity
//    - Resumes automatically when user input is detected
//
// Debouncing Behavior Example:
// With windowChangeDebounceSeconds=5, intervalSeconds=5:
// t=0s:  Window change → Screenshot taken
// t=5s:  Fixed interval → Screenshot taken
// t=6s:  Window change → Screenshot taken (>5s since last window change screenshot)
// t=7s:  Window change → No screenshot (only 1s since last window change screenshot)
// t=10s: Fixed interval → Screenshot taken (continues on schedule)
// t=11s: Window change → Screenshot taken (>5s since last window change screenshot)
class WindowChangeScreenshotTimingStrategy : public ScreenshotTimingStrategy, public WindowsForegroundHookListener
{
    friend class ScreenshotEventSourceBuilder; // needs to update source ptr
  public:
    WindowChangeScreenshotTimingStrategy(uint32_t intervalSeconds, uint32_t pauseAfterIdleSeconds,
                                         uint32_t windowChangeDebounceSeconds)
        : intervalSeconds(intervalSeconds), pauseAfterIdleSeconds(pauseAfterIdleSeconds),
          windowChangeDebounceSeconds(windowChangeDebounceSeconds)
    {
    }

    virtual ~WindowChangeScreenshotTimingStrategy();

    virtual bool screenshotThreadFunction(ScreenshotEventSource *source) override;

  private:
    // Pointer to the source that is capturing screenshots
    // Raw ptr, so be careful to check before using
    std::optional<ScreenshotEventSource *> source;

  private:
    // Time between regular interval screenshots
    uint32_t intervalSeconds;

    // Time of inactivity before pausing all screenshots
    uint32_t pauseAfterIdleSeconds;

    // Minimum time between window-change triggered screenshots
    // Prevents excessive screenshots during rapid window switching
    uint32_t windowChangeDebounceSeconds;
    // Time of last window change that triggered a screenshot
    uint32_t lastWindowChangeScreenshotTime;

    // Windows event hook callback that processes window focus change events
    // This is called by the system when relevant window events occur
    void onForegroundEvent(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd, LONG idObject, LONG idChild,
                           DWORD dwEventThread, DWORD dwmsEventTime) override;
};

// Take a screenshot every N seconds with a configurable idle timeout
class FixedIntervalScreenshotTimingStrategy : public ScreenshotTimingStrategy
{
  public:
    FixedIntervalScreenshotTimingStrategy(uint32_t intervalSeconds, uint32_t pauseAfterIdleSeconds)
        : intervalSeconds(intervalSeconds), pauseAfterIdleSeconds(pauseAfterIdleSeconds)
    {
    }

    virtual bool screenshotThreadFunction(ScreenshotEventSource *source) override;

  private:
    // Number of seconds between screenshots
    uint32_t intervalSeconds;

    // Number of seconds that the user can be idle before we pause taking screenshots ("Idle" meaning that the
    // user has not performed any keyboard or mouse input)
    uint32_t pauseAfterIdleSeconds;
};