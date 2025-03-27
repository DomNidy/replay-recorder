#include <windows.h>
#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include "event_sink.h"
#include "screenshot_event_source.h"
#include "user_input_event_source.h"
#include "user_window_activity_event_source.h"
#include "utils/logging.h"
#include "windows_hook_manager.h"

std::atomic<DWORD> g_mainThreadId{0};
void signalHandler(int signal)
{
    if (signal != SIGINT)
    {
        return;
    }

    LOG_DEBUG("---Graceful shutdown signal received in thread {} ---", GetCurrentThreadId());

    if (g_mainThreadId == 0)
    {
        LOG_ERROR("Main thread id not set, cannot send WM_QUIT message to it");
        std::exit(signal);
    }

    if (!PostThreadMessage(g_mainThreadId, WM_QUIT, 0, 0))
    {
        LOG_ERROR("Error posting WM_QUIT message to main thread: {}", static_cast<int>(GetLastError()));
        std::exit(signal);
    }

    LOG_DEBUG("WM_QUIT message posted to main thread ({})", g_mainThreadId.load());
}

int main(int argc, char** argv)
{
    RP::Logging::initLogging(spdlog::level::debug);
    g_mainThreadId = GetCurrentThreadId();
    LOG_DEBUG("Main thread id: {}", g_mainThreadId.load());
    std::signal(SIGINT, signalHandler);

    // Initialize the windows hook manager (should be done before creating any event sources and in the main thread)
    Replay::Windows::WindowsHookManager::getInstance();

    // Initialize event sink
    auto eventSink = std::make_shared<EventSink>("out.txt");

    // Create EventSources to monitor user activity
    auto inputEventSource = std::make_shared<UserInputEventSource>();
    auto windowActivityEventSource = std::make_shared<UserWindowActivityEventSource>();
    auto screenshotEventSource =
        ScreenshotEventSourceBuilder()
            .withScreenshotSerializationStrategy(ScreenshotSerializationStrategyType::FilePath)
            .withScreenshotTimingStrategy(std::make_shared<WindowChangeScreenshotTimingStrategy>(
                std::chrono::seconds(60), std::chrono::seconds(60), std::chrono::seconds(5), std::chrono::seconds(2)))
            .withScreenshotOutputDirectory(std::filesystem::path("./replay-screenshots"))
            .build();

    inputEventSource->initializeSource(eventSink);
    windowActivityEventSource->initializeSource(eventSink);
    screenshotEventSource->initializeSource(eventSink);

    BOOL ret;
    MSG msg;
    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (ret == -1)
        {
            throw std::runtime_error("Error retrieving message with GetMessage(), hWnd is probably an "
                                     "invalid window handle, or lpMsg is an invalid pointer! Error code:" +
                                     GetLastError());
        }
    }

    // Print ref counts of all event sources
    LOG_DEBUG("--- Shutting down... ---");
    LOG_DEBUG("InputEventSource ref count: {}", inputEventSource.use_count());
    LOG_DEBUG("WindowActivityEventSource ref count: {}", windowActivityEventSource.use_count());
    LOG_DEBUG("ScreenshotEventSource ref count: {}", screenshotEventSource.use_count());
}