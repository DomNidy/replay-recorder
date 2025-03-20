#include "user_window_activity_event_source.h"

#include <ctime>
#include <spdlog/spdlog.h>
#include <sstream>

#include "event_sink.h"
#include "utils/timestamp_utils.h"
#include "screenshot_serialization_strategy.h"

UserWindowActivityEventSource::~UserWindowActivityEventSource()
{
    spdlog::debug("UserWindowActivityEventSource::~UserWindowActivityEventSource: Destructor called");
}

void UserWindowActivityEventSource::uninitializeSource()
{
    spdlog::debug("Uninitializing UserWindowActivityEventSource in thread id: {}", GetCurrentThreadId());

    WindowsHookManager::getInstance().unregisterForegroundHookListener(shared_from_this());

    spdlog::info("UserWindowActivityEventSource successfully unregistered from WindowsHookManager");
}

void UserWindowActivityEventSource::initializeSource(std::weak_ptr<EventSink> inSink)
{
    outputSink = inSink;
    if (outputSink.expired())
    {
        throw std::runtime_error(RP_ERR_INITIALIZED_WITH_NULLPTR_EVENT_SINK);
    }

    spdlog::debug("UserWindowActivityEventSource::initializeSource called");

    // Add windows hook
    WindowsHookManager::getInstance().registerForegroundHookListener(shared_from_this());
    spdlog::debug("UserWindowActivityEventSource successfully installed window event hook");
}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wineventproc
void UserWindowActivityEventSource::onForegroundEvent(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd,
                                                      LONG idObject, LONG idChild, DWORD dwEventThread,
                                                      DWORD dwmsEventTime)
{
    std::string windowTitle;
    spdlog::debug("UserWindowActivityEventSource::onForegroundEvent called with event: {}", event);
    if (event == EVENT_SYSTEM_FOREGROUND && getWindowTitle(hWnd, windowTitle))
    {
        // Special separator token, produced when focus enters and exits a window
        if (windowTitle != "Task Switching")
        {
            std::time_t now = std::time(nullptr);
            std::string timestampString = RP::Utils::formatTimestampToLLMReadable(std::localtime(&now));

            std::ostringstream oss;
            oss << "\n" << WINDOW_CHANGE_TOKEN << "\"" << windowTitle << "\" TIMESTAMP: " << timestampString << WINDOW_CHANGE_END_TOKEN << "\n";
            *outputSink.lock() << oss.str().data();
        }
    }
}

bool UserWindowActivityEventSource::getWindowTitle(HWND hWindow, std::string &destStr)
{
    char processName[MAX_PATH] = "";
    if (GetWindowTextA(hWindow, processName, MAX_PATH) == 0)
    {
        return false;
    }

    destStr = processName;
    return true;
}
