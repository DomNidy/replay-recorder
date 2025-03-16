#include "screenshot_timing_strategy.h"
#include <algorithm>
#include <string>
#include <vector>
#include "screenshot_event_source.h"


bool FixedIntervalScreenshotTimingStrategy::screenshotThreadFunction(
    ScreenshotEventSource *source, CaptureScreenshotFunction captureScreenshotFunction) const
{
    spdlog::info("Screenshot timing strategy: Fixed interval");
    while (source && source->getIsRunning())
    {

        if (!isIdle(pauseAfterIdleSeconds))
        {
            spdlog::info("Capturing screenshot");
            (source->*captureScreenshotFunction)();
        }
        else
        {
            uint32_t timeSinceLastInput = getCurrentTime() - getLastInputTime();
            spdlog::info("Skipping screenshot because no input was detected for {} seconds", timeSinceLastInput / 1000);
        }

        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
    }
    return true;
}

bool WindowChangeScreenshotTimingStrategy::screenshotThreadFunction(
    ScreenshotEventSource *source, CaptureScreenshotFunction captureScreenshotFunction) const
{
    spdlog::info("Screenshot timing strategy: Window change");
    return true;
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