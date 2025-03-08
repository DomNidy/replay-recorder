#pragma once

#include <windows.h> // make sure to include windows.h before TlHelp32.h
#include <TlHelp32.h>
#include <memory>
#include "event_source.h"

/**
 * Singleton that records user input events (e.g., keyboard)
 */
class UserInputEventSource : public EventSource<UserInputEventSource>
{
    friend class EventSource<UserInputEventSource>;

private:
    /** Implementing singleton */
    UserInputEventSource() = default;
    ~UserInputEventSource() { uninitializeSource(); }

private:
    //~ Begin EventSource interface
    virtual void initializeSource(EventSink *inSink) override;
    virtual void uninitializeSource() override;
    //~ End EventSource interface

private:
    // The EventSink that registered us and we should write to
    static EventSink *outputSink;

private:
    // Store the handle to the keyboard input hook
    static HHOOK hKeyboardHook;

    // Hook procedure that is executed when keyboard input is received
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    // Enumerate all processes on the Windows system
    void enumerateWindowsProcesses() const;
};
