#include "windows_hook_manager.h"
#include <future> // Include for std::async
#include "utils/logging.h"

WindowsHookManager::WindowsHookManager()
{
    // TODO: Not sure if we should use WINEVENT_SKIPOWNPROCESS. Doesn't seem to change anything. Windows docs say:
    // "Prevents this instance of the hook from receiving the events that are generated by threads in this process. This
    // flag does not prevent threads from generating events." But since we are looking at system foreground events, I
    // don't think it matters.
    LOG_CLASS_DEBUG("WindowsHookManager", "Constructor called in thread {}", GetCurrentThreadId());

    // Register foreground hook proc
    hForegroundEventHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, // Range of events that we want our hook to handle
        NULL, // Handle to dll that contains the hook function. Null bc its in the same exe
        WindowsHookManager::WinForegroundEventProc, // Hook function to call when the event occurs
        0, // Process id that the hook receives events from (0 = all processes in current desktop)
        0, // Thread id that the hook receives events from (0 = all existing threads in current desktop)
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS); // Flags for the hook procedure

    LOG_CLASS_DEBUG("WindowsHookManager", "The foreground event hook is installed in thread {}", GetCurrentThreadId());
    if (!hForegroundEventHook)
    {
        LOG_CLASS_CRITICAL("WindowsHookManager", "Failed to install foreground event hook");
        throw std::runtime_error("WindowsHookManager: Failed to install foreground event hook");
    }
    else
    {
        LOG_CLASS_DEBUG("WindowsHookManager", "Foreground event hook handle: {}",
                        static_cast<void *>(hForegroundEventHook));
    }
}

WindowsHookManager::~WindowsHookManager()
{
    LOG_CLASS_DEBUG("WindowsHookManager", "Destructor called in thread {}", GetCurrentThreadId());
    LOG_CLASS_DEBUG("WindowsHookManager", "Uninstalling foreground event hook  {}",
                    static_cast<void *>(hForegroundEventHook));

    BOOL result = UnhookWinEvent(hForegroundEventHook);
    if (!result)
    {
        LOG_CLASS_WARN("WindowsHookManager",
                       "Failed to uninstall foreground event hook! This is "
                       "probably because the destructor is running in a different thread than the thread the hook was "
                       "registered in. Warning so I remember to fix this.");
    }
    else
    {
        LOG_CLASS_DEBUG("WindowsHookManager", "Uninstalled foreground event hook!");
    }
}

void CALLBACK WindowsHookManager::WinForegroundEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd,
                                                         LONG idObject, LONG idChild, DWORD dwEventThread,
                                                         DWORD dwmsEventTime)
{
    LOG_CLASS_DEBUG("WindowsHookManager", "got event: {}", event);

    // Run the callbacks in separate threads to avoid blocking the windows UI thread
    // TODO: Is it bad to create a bunch of threads here like this? Seems like it might be. Use thread pool?
    for (const auto &listener : WindowsHookManager::getInstance().foregroundHookListeners)
    {
        std::thread([&]() {
            LOG_CLASS_INFO("WindowsHookManager", "calling listener");
            if (!listener)
            {
                LOG_CLASS_ERROR("WindowsHookManager", "listener is null");
                return;
            }
            listener->onForegroundEvent(hWinEventHook, event, hWnd, idObject, idChild, dwEventThread, dwmsEventTime);
        }).detach();
    }
}

void WindowsHookManager::registerForegroundHookListener(std::shared_ptr<WindowsForegroundHookListener> listener)
{
    LOG_CLASS_DEBUG("WindowsHookManager", "registering a listener");
    foregroundHookListeners.push_back(listener);
}

void WindowsHookManager::unregisterForegroundHookListener(std::shared_ptr<WindowsForegroundHookListener> listener)
{
    LOG_CLASS_DEBUG("WindowsHookManager", "unregistering a listener");
    foregroundHookListeners.erase(std::remove(foregroundHookListeners.begin(), foregroundHookListeners.end(), listener),
                                  foregroundHookListeners.end());
}
