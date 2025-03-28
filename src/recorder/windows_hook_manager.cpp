#include "windows_hook_manager.h"
#include "utils/logging.h"

LRESULT CALLBACK Replay::Windows::WindowsHookManager::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        KeyboardInputEventData eventData(nCode, wParam, lParam);

        // Get the class data entry for the keyboard observer class
        auto keyboardInputObserverClassData =
            WindowsHookManager::getInstance()
                .getObserverClassDataEntry<KeyboardInputObserver, KeyboardInputEventData>();
        assert(keyboardInputObserverClassData != nullptr && "Failed to find keyboard input observer class data entry");

        // Push the event payload to the event queue
        // TODO: I don't really know if we need to still lock the mutex here. It seems like the answer is yes, perhaps
        // because non-atomic pushing could lead to undefined behavior? It's not ideal, but might be necessary.
        {
            std::lock_guard<std::mutex> lock(keyboardInputObserverClassData->eventQueueMutex);
            keyboardInputObserverClassData->eventQueue.emplace(std::make_tuple(eventData));
            keyboardInputObserverClassData->eventQueueConditionVariable.notify_one();
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void CALLBACK Replay::Windows::WindowsHookManager::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd,
                                                                LONG idObject, LONG idChild, DWORD idEventThread,
                                                                DWORD dwmsEventTime)
{
    if (event == EVENT_SYSTEM_FOREGROUND)
    {
        auto focusObserverClassData =
            WindowsHookManager::getInstance().getObserverClassDataEntry<FocusObserver, HWND>();
        assert(focusObserverClassData != nullptr);

        // Push the event payload to the event queue
        {
            std::lock_guard<std::mutex> lock(focusObserverClassData->eventQueueMutex);
            focusObserverClassData->eventQueue.emplace(std::make_tuple(hwnd));
            focusObserverClassData->eventQueueConditionVariable.notify_one();
        }
    }
}

Replay::Windows::KeyboardInputEventData::KeyboardInputEventData() : nCode(0), wParam(0), lParam(0)
{
}

Replay::Windows::KeyboardInputEventData::KeyboardInputEventData(int code, WPARAM wp, LPARAM lp)
    : nCode(code), wParam(wp), lParam(lp)
{
}
