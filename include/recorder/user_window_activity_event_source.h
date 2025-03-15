#pragma once

#include <windows.h>

#include <string>

#include "event_source.h"

class UserWindowActivityEventSource : public EventSource
{

  public:
    UserWindowActivityEventSource() = default;
    ~UserWindowActivityEventSource()
    {
        uninitializeSource();
    }

  private:
    virtual void initializeSource(std::weak_ptr<EventSink> inSink) override;
    virtual void uninitializeSource() override;

    std::weak_ptr<EventSink> outputSink;

    // Returns the name of the process corresponding to the focused window
    bool getWindowTitle(HWND hWindow, std::string &destStr);

  private:
    // Hook procedure that is executed in response to window events
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hWnd, LONG idObject, LONG idChild,
                                      DWORD dwEventThread, DWORD dwmsEventTime);

    // Store the haandle to the WinEventProc
    // static bc Windows API
    static HWINEVENTHOOK hWinEventHook;
};