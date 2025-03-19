#include "screenshot_timing_strategy.h"
#include <algorithm>
#include <string>
#include <vector>
#include "screenshot_event_source.h"

bool FixedIntervalScreenshotTimingStrategy::screenshotThreadFunction(ScreenshotEventSource *source)
{
    spdlog::info("Screenshot timing strategy: Fixed interval");
    while (source && source->getIsRunning())
    {

        if (!isIdle(pauseAfterIdleSeconds))
        {
            spdlog::info("Capturing screenshot");
            source->captureScreenshot();
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

bool WindowChangeScreenshotTimingStrategy::screenshotThreadFunction(ScreenshotEventSource *source)
{
    spdlog::debug("Screenshot timing strategy: Window change, trying to register hook...");

    WindowsHookManager::getInstance().registerForegroundHookListener(shared_from_this());

    spdlog::debug("WindowChangeScreenshotTimingStrategy: Hook installed successfully");

    return true;
}

void WindowChangeScreenshotTimingStrategy::onForegroundEvent(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd,
                                                             LONG idObject, LONG idChild, DWORD dwEventThread,
                                                             DWORD dwmsEventTime)
{
    spdlog::debug("--- WindowChangeScreenshotTimingStrategy: onForegroundEvent called with event: {} ----", event);

    if (source.has_value())
    {
        spdlog::debug("WindowChangeScreenshotTimingStrategy: onForegroundEvent called thread id: {}",
                      GetCurrentThreadId());
        uint32_t timeSinceLastWindowChange = dwmsEventTime - lastWindowChangeScreenshotTime;
        if (timeSinceLastWindowChange >= windowChangeDebounceSeconds * 1000)
        {
            lastWindowChangeScreenshotTime = dwmsEventTime;
            // do a little delay before ss to ensure it captures the new window
            std::this_thread::sleep_for(std::chrono::milliseconds(1250));
            if (source.has_value())
            {
                source.value()->captureScreenshot();
            }
        }
    }
    spdlog::debug("--- WindowChangeScreenshotTimingStrategy: onForegroundEvent finished ----");
}

WindowChangeScreenshotTimingStrategy::~WindowChangeScreenshotTimingStrategy()
{
    spdlog::debug("WindowChangeScreenshotTimingStrategy: Destructor called, trying to unregister hook...");

    // TODO: THIS THROWS A BAD WEAK PTR EXCEPTION, ITS BC OF THE USAGE OF SHARED_FROM_THIS. MAYBE NOT GOOD TO CALL IT IN
    // DESTRUCTOR???
    // WindowsHookManager::getInstance().unregisterForegroundHookListener(shared_from_this());
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