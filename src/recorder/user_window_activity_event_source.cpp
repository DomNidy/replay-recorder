#include "user_window_activity_event_source.h"

#include <ctime>
#include <spdlog/spdlog.h>
#include <sstream>

#include "event_sink.h"

HWINEVENTHOOK UserWindowActivityEventSource::hWinEventHook = NULL;
UserWindowActivityEventSource *currentInstance = nullptr;

// Date formatting utility functions
// move this to a util lib or header
std::string _formatTimestampGetOrdinalDay(int day)
{
    switch (day)
    {
    case 1:
        return "first";
    case 2:
        return "second";
    case 3:
        return "third";
    case 4:
        return "fourth";
    case 5:
        return "fifth";
    case 6:
        return "sixth";
    case 7:
        return "seventh";
    case 8:
        return "eighth";
    case 9:
        return "ninth";
    case 10:
        return "tenth";
    case 11:
        return "eleventh";
    case 12:
        return "twelfth";
    case 13:
        return "thirteenth";
    case 14:
        return "fourteenth";
    case 15:
        return "fifteenth";
    case 16:
        return "sixteenth";
    case 17:
        return "seventeenth";
    case 18:
        return "eighteenth";
    case 19:
        return "nineteenth";
    case 20:
        return "twentieth";
    case 21:
        return "twenty-first";
    case 22:
        return "twenty-second";
    case 23:
        return "twenty-third";
    case 24:
        return "twenty-fourth";
    case 25:
        return "twenty-fifth";
    case 26:
        return "twenty-sixth";
    case 27:
        return "twenty-seventh";
    case 28:
        return "twenty-eighth";
    case 29:
        return "twenty-ninth";
    case 30:
        return "thirtieth";
    case 31:
        return "thirty-first";
    default:
        return ""; // Handle invalid day if necessary
    }
}

std::string _formatTimestampToLLMReadable(std::tm *time)
{
    if (!time)
    {
        throw std::runtime_error("Received nullptr instead of std::tm");
    }

    // Get ordinal day (e.g., "first", "thirtieth")
    std::string dayOrdinal = _formatTimestampGetOrdinalDay(time->tm_mday);

    char yearMonth[100];
    std::strftime(yearMonth, sizeof(yearMonth), "%Y %B", time);

    char timePart[10];
    std::strftime(timePart, sizeof(timePart), "%H:%M", time);

    std::stringstream ss;
    ss << yearMonth << " " << dayOrdinal << " " << timePart;

    return ss.str();
}

void UserWindowActivityEventSource::initializeSource(std::weak_ptr<EventSink> inSink)
{
    outputSink = inSink;
    if (outputSink.expired())
    {
        throw std::runtime_error(RP_ERR_INITIALIZED_WITH_NULLPTR_EVENT_SINK);
    }

    if (hWinEventHook != NULL || currentInstance != nullptr)
    {
        throw std::runtime_error(
            std::string("Tried to initialize a UserWindowActivityEventSource, but one was already initialized earlier."
                        "Only a single UserWindowActivityEventSource may exist at any given time. hWinEventHook must "
                        "be NULL, and currentInstance must be nullptr!") +
            "\nWas hWinEventHook null?: " + (hWinEventHook == NULL ? "Yes" : "No") +
            "\nWas currentInstance nullptr?: " + (currentInstance == nullptr ? "Yes" : "No"));
    }

    // Add windows hook
    hWinEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
                                    NULL, // Procedure is not in another module (its in this one)
                                    WinEventProc, 0, 0,
                                    WINEVENT_OUTOFCONTEXT // Invoke callback immediately
    );

    if (!hWinEventHook)
    {
        throw std::runtime_error("Failed to install window event hook: " + std::to_string(GetLastError()));
    }
    currentInstance = this;

    spdlog::info("UserWindowActivityEventSource successfully installed keyboard hook");
}

void UserWindowActivityEventSource::uninitializeSource()
{
    assert(hWinEventHook != NULL);

    UnhookWinEvent(hWinEventHook);
    hWinEventHook = NULL;
    currentInstance = nullptr;
    spdlog::info("UserWindowActivityEventSource successfully uninstalled window event hook");
}

// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wineventproc
void CALLBACK UserWindowActivityEventSource::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd,
                                                          LONG idObject, LONG idChild, DWORD dwEventThread,
                                                          DWORD dwmsEventTime)
{
    assert(currentInstance != nullptr &&
           "UserWindowActivityEventSource::WinEventProc ran, but currentInstance was nullptr. This should "
           "never happen, as the currentInstance should be set to null when the hook is removed.");

    std::string windowTitle;

    if (event == EVENT_SYSTEM_FOREGROUND && currentInstance->getWindowTitle(hWnd, windowTitle))
    {
        // Special separator token, produced when focus enters and exits a window
        if (windowTitle != "Task Switching")
        {
            std::time_t now = std::time(nullptr);
            std::string timestampString = _formatTimestampToLLMReadable(std::localtime(&now));

            std::ostringstream oss;
            oss << "\n[CHANGE WINDOW: \"" << windowTitle << "\", TIMESTAMP: " << timestampString << "]\n";
            *currentInstance->outputSink.lock() << oss.str().data();
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
