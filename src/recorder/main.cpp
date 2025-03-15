#include <windows.h>

#include <csignal>
#include <iostream>
#include <memory>
#include "event_sink.h"
#include "screenshot_event_source.h"
#include "spdlog/spdlog.h"
#include "user_input_event_source.h"
#include "user_window_activity_event_source.h"

int main(int argc, char **argv)
{
    // Create EventSink (receives events pertaining to the user's activity)
    EventSink eventSink = EventSink("out.txt");

    // Create EventSources to monitor user activity
    auto inputEventSource = std::make_unique<UserInputEventSource>();
    auto windowActivityEventSource = std::make_unique<UserWindowActivityEventSource>();
    auto screenshotEventSource = std::make_unique<ScreenshotEventSource>(
        ScreenshotEventSourceConfig(60, "./replay-screenshots", ScreenshotSerializationStrategyType::FilePath));

    // Add sources to sink
    eventSink.addSource(std::move(inputEventSource));
    eventSink.addSource(std::move(windowActivityEventSource));
    eventSink.addSource(std::move(screenshotEventSource));

    BOOL ret;
    MSG msg;
    while ((ret = GetMessage(&msg, NULL, WM_KEYFIRST, WM_MOUSELAST)) != 0)
    {
        if (ret == -1)
        {
            throw std::runtime_error("Error retrieving message with GetMessage(), hWnd is probably an "
                                     "invalid window handle, or lpMsg is an invalid pointer! Error code:" +
                                     GetLastError());
        }
    }
}