#pragma once

#include <Windows.h>
#include <memory>
#include <vector>

// ABC that any EventSource that wants to receive windows foreground hook events must implement
class WindowsForegroundHookListener : public std::enable_shared_from_this<WindowsForegroundHookListener>
{
  public:
    WindowsForegroundHookListener() = default;
    virtual ~WindowsForegroundHookListener() = default;

    // Called when a windows event is received
    virtual void onForegroundEvent(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd, LONG idObject, LONG idChild,
                                   DWORD dwEventThread, DWORD dwmsEventTime) = 0;
};

// We install a single hook only. Any event source that needs to be notified of windows events can register
// themselves with this, and we'll loop over all event sources each time the windows hook is ran, executing all
// registered event sources.
// Note: This is thread sensitive, and will only receive events if the calling thread has a message loop. To be careful,
// just create in the main thread.
class WindowsHookManager
{
  public:
    static WindowsHookManager &getInstance()
    {
        static WindowsHookManager instance;
        return instance;
    }

    WindowsHookManager(const WindowsHookManager &) = delete;
    WindowsHookManager &operator=(const WindowsHookManager &) = delete;

    void registerForegroundHookListener(std::shared_ptr<WindowsForegroundHookListener> listener);
    void unregisterForegroundHookListener(std::shared_ptr<WindowsForegroundHookListener> listener);

  private:
    std::vector<std::shared_ptr<WindowsForegroundHookListener>> foregroundHookListeners;

  private:
    // Callback and handle to foreground event hook
    HWINEVENTHOOK hForegroundEventHook;
    static void CALLBACK WinForegroundEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd, LONG idObject,
                                                LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

    WindowsHookManager();
    ~WindowsHookManager() = default;
};