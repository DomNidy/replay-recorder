#include "screenshot_timing_strategy.h"
#include <algorithm>
#include <string>
#include <vector>
#include "screenshot_event_source.h"
#include "utils/logging.h"
#include "utils/error_messages.h"

bool FixedIntervalScreenshotTimingStrategy::screenshotThreadFunction()
{
    LOG_CLASS_INFO("FixedIntervalScreenshotTimingStrategy", "Screenshot timing strategy: Fixed interval");
    auto lockedSource = source.lock();
    if (!lockedSource)
    {
        LOG_CLASS_ERROR("FixedIntervalScreenshotTimingStrategy", "Source is null, cannot capture screenshot");
        return false;
    }
    while (lockedSource && lockedSource->getIsRunning())
    {
        if (!isIdle(pauseAfterIdleSeconds))
        {
            LOG_CLASS_INFO("FixedIntervalScreenshotTimingStrategy", "Capturing screenshot");
            lockedSource->captureScreenshot();
        }
        else
        {
            uint32_t timeSinceLastInput = getCurrentTime() - getLastInputTime();
            LOG_CLASS_INFO("FixedIntervalScreenshotTimingStrategy",
                           "Skipping screenshot because no input was detected for {} seconds",
                           timeSinceLastInput / 1000);
        }

        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
    }
    return true;
}

bool WindowChangeScreenshotTimingStrategy::screenshotThreadFunction()
{
    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy",
                    "Screenshot timing strategy screenshotThreadFunction called, trying register ourselves as a "
                    "FocusObserver...");

    Replay::Windows::WindowsHookManager::getInstance().registerObserver<Replay::Windows::FocusObserver>(
        shared_from_this());

    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy",
                    "Successfully registered as a FocusObserver with WindowsHookManager");

    return true;
}

void WindowChangeScreenshotTimingStrategy::onFocusChange(HWND hwnd)
{
    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy", "Entered window change strategy");

    auto lockedSource = source.lock();
    if (lockedSource)
    {
        // Check if we should debounce the screenshot
        auto timeSinceLastWindowChange = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::time_point<std::chrono::steady_clock>(std::chrono::milliseconds(getCurrentTime())) -
            lastWindowChangeScreenshotTime);

        if (timeSinceLastWindowChange < windowChangeDebounceSeconds)
        {
            LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy",
                            "Debouncing screenshot because it was taken too recently ({}s < {}s)",
                            timeSinceLastWindowChange.count(), windowChangeDebounceSeconds.count());
            return;
        }

        // Update the last window change screenshot time to now
        lastWindowChangeScreenshotTime = std::chrono::steady_clock::now();

        // do a little delay before ss to ensure it captures the new window
        std::this_thread::sleep_for(windowChangeScreenshotDelaySeconds);
        lockedSource->captureScreenshot();
    }
    else
    {
        LOG_CLASS_ERROR("WindowChangeScreenshotTimingStrategy", "Source is null, cannot capture screenshot");
    }
    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy", "onForegroundEvent finished");
}

WindowChangeScreenshotTimingStrategy::~WindowChangeScreenshotTimingStrategy()
{
    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy", "Destructor called, trying to unregister hook...");
    try
    {
        auto self = shared_from_this();
        Replay::Windows::WindowsHookManager::getInstance().unregisterObserver<Replay::Windows::FocusObserver>(self);
    }
    catch (const std::bad_weak_ptr&)
    {
        LOG_CLASS_WARN("WindowChangeScreenshotTimingStrategy", "{}", RP::ErrorMessages::OBSERVER_UNREGISTER_FAILED);
    }
}

uint32_t ScreenshotTimingStrategy::getLastInputTime() const
{
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    GetLastInputInfo(&lii);
    return lii.dwTime;
}

uint32_t ScreenshotTimingStrategy::getCurrentTime() const
{
    return GetTickCount();
}

bool ScreenshotTimingStrategy::isIdle(uint32_t idleThresholdSeconds) const
{
    uint32_t currentTime = getCurrentTime();
    uint32_t lastInputTime = getLastInputTime();
    uint32_t timeSinceLastInput;

    // Handle wrap around edge case
    if (currentTime >= lastInputTime)
    {
        timeSinceLastInput = currentTime - lastInputTime;
    }
    else
    {
        timeSinceLastInput = (MAXDWORD - lastInputTime) + currentTime + 1;
    }

    return timeSinceLastInput >= idleThresholdSeconds * 1000;
}