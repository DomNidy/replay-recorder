#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
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

class ScreenshotTimingStrategy
{
  public:
    virtual ~ScreenshotTimingStrategy()
    {
        LOG_CLASS_DEBUG("ScreenshotTimingStrategy", "Destructor called");
    }
    ScreenshotTimingStrategy() = default;

    virtual bool screenshotThreadFunction(ScreenshotEventSource* source) = 0;

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
//    - A configurable `windowChangeScreenshotDelaySeconds` to delay the screenshot after a window change (otherwise it
//      may not capture the new window)
//    - Only triggers on actual window focus changes, not other window events
//
// 2. Fixed Interval Backup: When no window changes are detected for a period of time, switches to taking screenshots at
//    fixed intervals
//    - Ensures activity is captured even when the user stays in one window for extended periods
//    - Interval screenshots continue regardless of window change debouncing
//
// 3. Idle Detection: Pauses all screenshot capture during periods of user inactivity
//    - Resumes automatically when user input is detected
//
// Debouncing Behavior Example:
// With windowChangeDebounceSeconds=5, intervalSeconds=5, windowChangeScreenshotDelaySeconds=2:
// t=0s:  Window change → Schedule a screenshot to be taken 2 seconds from now (windowChangeScreenshotDelaySeconds)
// t=2s:  ...           → Screenshot taken
// t=5s:  Fixed interval → Screenshot taken
// t=6s:  Window change → Screenshot taken (>5s since last window change screenshot)
// t=7s:  Window change → No screenshot (only 1s since last window change screenshot)
// t=10s: Fixed interval → Screenshot taken (continues on schedule)
// t=11s: Window change → Screenshot taken (>5s since last window change screenshot)
class WindowChangeScreenshotTimingStrategy : public ScreenshotTimingStrategy,
                                             public Replay::Windows::FocusObserver,
                                             public std::enable_shared_from_this<WindowChangeScreenshotTimingStrategy>
{
    friend class ScreenshotEventSourceBuilder; // needs to update source ptr
  public:
    WindowChangeScreenshotTimingStrategy(
        std::chrono::seconds intervalSeconds = std::chrono::seconds(60),
        std::chrono::seconds pauseAfterIdleSeconds = std::chrono::seconds(60),
        std::chrono::seconds windowChangeDebounceSeconds = std::chrono::seconds(5),
        std::chrono::seconds windowChangeScreenshotDelaySeconds = std::chrono::seconds(2))
        : intervalSeconds(intervalSeconds),
          pauseAfterIdleSeconds(pauseAfterIdleSeconds),
          windowChangeDebounceSeconds(windowChangeDebounceSeconds),
          windowChangeScreenshotDelaySeconds(windowChangeScreenshotDelaySeconds)
    {
    }

    virtual ~WindowChangeScreenshotTimingStrategy();

    //~ Begin ScreenshotTimingStrategy interface
    virtual bool screenshotThreadFunction(ScreenshotEventSource* source) override;
    //~ End ScreenshotTimingStrategy interface

  private:
    //~ Begin Replay::Windows::FocusObserver interface
    void onFocusChange(HWND hwnd) override;
    //~ End Replay::Windows::FocusObserver interface

  private:
    // Pointer to the source that is capturing screenshots
    // Raw ptr, so be careful to check before using
    std::optional<ScreenshotEventSource*> source;

  private:
    // Time between interval screenshots (for the fixed interval backup strategy)
    std::chrono::seconds intervalSeconds;

    // Time of inactivity before pausing all screenshots
    std::chrono::seconds pauseAfterIdleSeconds;

    // Do not take a screenshot in response to a window change if we took a screenshot in the last this many seconds
    // Prevents excessive screenshots during rapid window switching
    std::chrono::seconds windowChangeDebounceSeconds;

    // Time to delay the screenshot after a window change
    std::chrono::seconds windowChangeScreenshotDelaySeconds;

  private:
    // Time of last window change that triggered a screenshot
    std::chrono::time_point<std::chrono::steady_clock> lastWindowChangeScreenshotTime;
};

// Take a screenshot every N seconds with a configurable idle timeout
class FixedIntervalScreenshotTimingStrategy : public ScreenshotTimingStrategy
{
  public:
    FixedIntervalScreenshotTimingStrategy(uint32_t intervalSeconds = 60, uint32_t pauseAfterIdleSeconds = 60)
        : intervalSeconds(intervalSeconds), pauseAfterIdleSeconds(pauseAfterIdleSeconds)
    {
    }
    
    ~FixedIntervalScreenshotTimingStrategy()
    {
        LOG_CLASS_DEBUG("FixedIntervalScreenshotTimingStrategy", "Destructor called");
    }

    //~ Begin ScreenshotTimingStrategy interface
    virtual bool screenshotThreadFunction(ScreenshotEventSource* source) override;
    //~ End ScreenshotTimingStrategy interface

  private:
    // Number of seconds between screenshots
    uint32_t intervalSeconds;

    // Number of seconds that the user can be idle before we pause taking screenshots ("Idle" meaning that the
    // user has not performed any keyboard or mouse input)
    uint32_t pauseAfterIdleSeconds;
};