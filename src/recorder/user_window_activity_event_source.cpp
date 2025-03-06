#include "recorder/user_window_activity_event_source.h"
#include "recorder/event_sink.h"
#include <ctime>
#include <sstream>

EventSink *UserWindowActivityEventSource::outputSink = nullptr;

void UserWindowActivityEventSource::initializeSource(EventSink *inSink)
{
    outputSink = inSink;
    if (outputSink == nullptr)
    {
        throw std::runtime_error("initializeSource called with inSink == nullptr");
    }

    // Add windows hook
    hWinEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_SYSTEM_FOREGROUND,
        NULL, // Procedure is not in another module (its in this one)
        WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT // Invoke callback immediately
    );

    if (!hWinEventHook)
    {
        throw std::runtime_error("Failed to install window event hook: " + std::to_string(GetLastError()));
    }
}

void UserWindowActivityEventSource::uninitializeSource()
{
    assert(hWinEventHook != NULL);

    UnhookWinEvent(hWinEventHook);
    hWinEventHook = NULL;
    std::cout << "UserWindowActivityEventSource successfully uninstalled window event hook" << std::endl;
}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wineventproc
void CALLBACK UserWindowActivityEventSource::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{

    std::string windowTitle;

    if (event == EVENT_SYSTEM_FOREGROUND && getInstance().getWindowTitle(hWnd, windowTitle))
    {
        // Special separator token, produced when focus enters and exits a window
        if (windowTitle != "Task Switching")
        {
            std::time_t now = std::time(nullptr);
            char timeStr[100];
            strftime(timeStr, sizeof(timeStr), "%Y%m%d%H%M%S", localtime(&now));

            std::ostringstream oss;
            oss << "\n[CHANGE WINDOW: \"" << windowTitle << "\", TIMESTAMP: " << timeStr << "]\n";
            *outputSink << oss.str().data();
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