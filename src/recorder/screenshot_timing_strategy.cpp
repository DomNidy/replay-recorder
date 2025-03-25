#include "screenshot_timing_strategy.h"
#include <algorithm>
#include <string>
#include <vector>
#include "screenshot_event_source.h"
#include "utils/logging.h"

bool FixedIntervalScreenshotTimingStrategy::screenshotThreadFunction(ScreenshotEventSource* source)
{
    LOG_CLASS_INFO("FixedIntervalScreenshotTimingStrategy", "Screenshot timing strategy: Fixed interval");
    while (source && source->getIsRunning())
    {

        if (!isIdle(pauseAfterIdleSeconds))
        {
            LOG_CLASS_INFO("FixedIntervalScreenshotTimingStrategy", "Capturing screenshot");
            source->captureScreenshot();
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

bool WindowChangeScreenshotTimingStrategy::screenshotThreadFunction(ScreenshotEventSource* source)
{
    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy",
                    "Screenshot timing strategy: Window change, trying to register hook...");

    Replay::Windows::WindowsHookManager::getInstance().registerObserver<Replay::Windows::FocusObserver>(
        shared_from_this());

    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy", "Hook installed successfully");

    return true;
}

void WindowChangeScreenshotTimingStrategy::onFocusChange(HWND hwnd)
{
    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy", "Entered window change strategy");

    if (source.has_value())
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
        if (source.has_value())
        {
            source.value()->captureScreenshot();
        }
    }
    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy", "onForegroundEvent finished");
}

WindowChangeScreenshotTimingStrategy::~WindowChangeScreenshotTimingStrategy()
{
    LOG_CLASS_DEBUG("WindowChangeScreenshotTimingStrategy", "Destructor called, trying to unregister hook...");
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