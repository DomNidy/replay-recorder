#include "windows_hook_manager.h"
#include <spdlog/spdlog.h>

WindowsHookManager::WindowsHookManager()
{
    // Register foreground hook proc
    hForegroundEventHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL,
                                           WindowsHookManager::WinForegroundEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);

    spdlog::debug("WindowsHookManager: Installing foreground event hook in thread {}", GetCurrentThreadId());
    if (!hForegroundEventHook)
    {
        spdlog::error("WindowsHookManager: Failed to install foreground event hook");
    }
    else
    {
        spdlog::debug("WindowsHookManager: Installed a foreground event hook");
    }
}

void CALLBACK WindowsHookManager::WinForegroundEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd,
                                                         LONG idObject, LONG idChild, DWORD dwEventThread,
                                                         DWORD dwmsEventTime)
{
    spdlog::info("WindowsHookManager::WinForegroundEventProc: got event: {}", event);
    for (const auto &listener : WindowsHookManager::getInstance().foregroundHookListeners)
    {
        spdlog::info("WindowsHookManager::WinForegroundEventProc: calling listener");
        listener->onForegroundEvent(hWinEventHook, event, hWnd, idObject, idChild, dwEventThread, dwmsEventTime);
    }
}

void WindowsHookManager::registerForegroundHookListener(std::shared_ptr<WindowsForegroundHookListener> listener)
{
    spdlog::debug("WindowsHookManager::registerForegroundHookListener: registering a listener");
    foregroundHookListeners.push_back(listener);
}

void WindowsHookManager::unregisterForegroundHookListener(std::shared_ptr<WindowsForegroundHookListener> listener)
{
    spdlog::debug("WindowsHookManager::unregisterForegroundHookListener: unregistering listener");

    foregroundHookListeners.erase(std::remove(foregroundHookListeners.begin(), foregroundHookListeners.end(), listener),
                                  foregroundHookListeners.end());
}
