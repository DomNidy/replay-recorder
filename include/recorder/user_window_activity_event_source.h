#pragma once

#include <windows.h>

#include <string>

#include "event_source.h"
#include "windows_hook_manager.h"

class UserWindowActivityEventSource : public EventSource, public WindowsForegroundHookListener
{

  public:
    UserWindowActivityEventSource() = default;
    ~UserWindowActivityEventSource();

  private:
    virtual void initializeSource(std::weak_ptr<EventSink> inSink) override;
    virtual void uninitializeSource() override;

    std::weak_ptr<EventSink> outputSink;

    // Returns the name of the process corresponding to the focused window
    bool getWindowTitle(HWND hWindow, std::string &destStr);

  private:
    void onForegroundEvent(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd, LONG idObject, LONG idChild,
                           DWORD dwEventThread, DWORD dwmsEventTime) override;
};