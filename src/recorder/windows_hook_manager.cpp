#include "windows_hook_manager.h"
#include "utils/logging.h"

LRESULT CALLBACK Replay::Windows::WindowsHookManager::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        LOG_CLASS_INFO("WindowsHookManager", "KeyboardProc called");
        // Parse relevant data needed for event payload
        bool keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        DWORD vkCode = ((KBDLLHOOKSTRUCT*)lParam)->vkCode;

        // Get the class data entry for the keyboard observer class
        auto keyboardInputObserverClassData =
            WindowsHookManager::getInstance().getObserverClassDataEntry<KeyboardInputObserver, DWORD, bool>();
        assert(keyboardInputObserverClassData != nullptr);

        // Push the event payload to the event queue
        keyboardInputObserverClassData->eventQueue.emplace(std::make_tuple(vkCode, keyDown));
        keyboardInputObserverClassData->eventQueueConditionVariable.notify_one();
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

        focusObserverClassData->eventQueue.emplace(std::make_tuple(hwnd));
        focusObserverClassData->eventQueueConditionVariable.notify_one();
    }
}
