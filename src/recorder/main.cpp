#include <windows.h>

#include <csignal>
#include <iostream>
#include <memory>
#include "event_sink.h"
#include "screenshot_event_source.h"
#include "spdlog/spdlog.h"
#include "user_input_event_source.h"
#include "user_window_activity_event_source.h"

namespace
{
std::shared_ptr<EventSink> g_eventSink;
}

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        spdlog::info("Gracefully shutting down...");
        if (g_eventSink)
        {
            g_eventSink.reset();
        }
        exit(0);
    }
}

int main(int argc, char **argv)
{

    std::signal(SIGINT, signalHandler);
    g_eventSink = std::make_shared<EventSink>("out.txt");
    
    // Create EventSources to monitor user activity
    auto inputEventSource = std::make_unique<UserInputEventSource>();
    auto windowActivityEventSource = std::make_unique<UserWindowActivityEventSource>();
    auto screenshotEventSource =
        ScreenshotEventSourceBuilder()
            .withScreenshotSerializationStrategy(ScreenshotSerializationStrategyType::FilePath)
            .withScreenshotOutputDirectory(std::filesystem::path("./replay-screenshots"))
            .withScreenshotTimingStrategy(std::make_unique<FixedIntervalScreenshotTimingStrategy>(5, 5))
            .build();

    // Add sources to sink
    g_eventSink->addSource(std::move(inputEventSource));
    g_eventSink->addSource(std::move(windowActivityEventSource));
    g_eventSink->addSource(std::move(screenshotEventSource));

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