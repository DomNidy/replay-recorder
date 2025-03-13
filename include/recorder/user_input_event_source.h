#pragma once

#include <windows.h>

#include <TlHelp32.h>
#include <memory>

#include "event_source.h"

/**
 * Singleton that records user input events (e.g., keyboard)
 */
class UserInputEventSource : public EventSource
{
  public:
    UserInputEventSource() = default;

  private:
    ~UserInputEventSource()
    {
        uninitializeSource();
    }

  private:
    //~ Begin EventSource interface
    virtual void initializeSource(EventSink *inSink) override;
    virtual void uninitializeSource() override;
    //~ End EventSource interface

  private:
    // The EventSink that registered us and we should write to
    EventSink *outputSink;

    // Enumerate all processes on the Windows system
    void enumerateWindowsProcesses() const;

  private:
    // Store the handle to the keyboard input hook
    // Currently, only one keyboard hook can be registered
    // these are required to be static bc of Windows API.
    static HHOOK hKeyboardHook;

    // Hook procedure that is executed when keyboard input is received
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
};
