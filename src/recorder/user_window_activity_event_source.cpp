#include "user_window_activity_event_source.h"

#include <ctime>

#include <sstream>

#include "event_sink.h"
#include "utils/logging.h"
#include "utils/timestamp_utils.h"

UserWindowActivityEventSource::~UserWindowActivityEventSource()
{
    LOG_CLASS_DEBUG("UserWindowActivityEventSource", "Destructor called");
}

void UserWindowActivityEventSource::uninitializeSource()
{
    LOG_CLASS_DEBUG("UserWindowActivityEventSource", "Uninitializing in thread id: {}", GetCurrentThreadId());

    WindowsHookManager::getInstance().unregisterForegroundHookListener(shared_from_this());

    LOG_CLASS_INFO("UserWindowActivityEventSource", "Successfully unregistered from WindowsHookManager");
}

void UserWindowActivityEventSource::initializeSource(std::weak_ptr<EventSink> inSink)
{
    outputSink = inSink;
    if (outputSink.expired())
    {
        throw std::runtime_error(RP_ERR_INITIALIZED_WITH_NULLPTR_EVENT_SINK);
    }

    LOG_CLASS_DEBUG("UserWindowActivityEventSource", "initializeSource called");

    // Add windows hook
    WindowsHookManager::getInstance().registerForegroundHookListener(shared_from_this());
    LOG_CLASS_DEBUG("UserWindowActivityEventSource", "Successfully installed window event hook");
}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wineventproc
void UserWindowActivityEventSource::onForegroundEvent(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd,
                                                      LONG idObject, LONG idChild, DWORD dwEventThread,
                                                      DWORD dwmsEventTime)
{
    std::string windowTitle;
    LOG_CLASS_DEBUG("UserWindowActivityEventSource", "onForegroundEvent called with event: {}", event);
    if (event == EVENT_SYSTEM_FOREGROUND && getWindowTitle(hWnd, windowTitle))
    {
        // Special separator token, produced when focus enters and exits a window
        if (windowTitle != "Task Switching")
        {
            std::time_t now = std::time(nullptr);
            std::string timestampString = RP::Utils::formatTimestampToLLMReadable(std::localtime(&now));

            std::ostringstream oss;
            oss << "\n"
                << WINDOW_CHANGE_TOKEN << "\"" << windowTitle << "\" TIMESTAMP: " << timestampString
                << WINDOW_CHANGE_END_TOKEN << "\n";
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
