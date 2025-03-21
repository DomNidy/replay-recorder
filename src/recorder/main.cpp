#include <windows.h>

#include <csignal>
#include <iostream>
#include <memory>
#include "event_sink.h"
#include "screenshot_event_source.h"
#include "user_input_event_source.h"
#include "user_window_activity_event_source.h"
#include "utils/logging.h"
#include "windows_hook_manager.h"
namespace
{
std::shared_ptr<EventSink> g_eventSink;
}

void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        LOG_INFO("--- Gracefully shutting down in thread {} ---", GetCurrentThreadId());

        if (g_eventSink)
        {
            g_eventSink->uninitializeSink();
            g_eventSink.reset();
        }
        exit(0);
    }
}

int main(int argc, char **argv)
{
    RP::Logging::initLogging(spdlog::level::debug);
    std::signal(SIGINT, signalHandler);

    // Initialize the windows hook manager (should be done before creating any event sources and in the main thread)
    WindowsHookManager::getInstance();

    // Initialize event sink
    g_eventSink = std::make_shared<EventSink>("out.txt");

    LOG_DEBUG("Main thread id: {}", GetCurrentThreadId());

    // Create EventSources to monitor user activity
    auto inputEventSource = std::make_unique<UserInputEventSource>();
    auto windowActivityEventSource = std::make_shared<UserWindowActivityEventSource>();
    auto screenshotEventSource =
        ScreenshotEventSourceBuilder()
            .withScreenshotSerializationStrategy(ScreenshotSerializationStrategyType::FilePath)
            .withScreenshotTimingStrategy(std::make_shared<WindowChangeScreenshotTimingStrategy>(
                std::chrono::seconds(60), std::chrono::seconds(60), std::chrono::seconds(5), std::chrono::seconds(2)))
            .withScreenshotOutputDirectory(std::filesystem::path("./replay-screenshots"))
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