#pragma once

#include "event_sink.h"
#include "event_source.h"
#include <memory>
#include <windows.h>

/**
 * Singleton that records user input events (e.g., keyboard)
 */
class UserInputEventSource : public EventSource
{

private:
    /** Implementing singleton */
    UserInputEventSource() {}
    ~UserInputEventSource() { uninitializeSource(); }
    static UserInputEventSource *instance;

    /** Deleting copy constructor & assignment operator */
    UserInputEventSource(const UserInputEventSource &) = delete;
    UserInputEventSource &operator=(const UserInputEventSource &) = delete;

public:
    static UserInputEventSource *getInstance();

private:
    friend EventSink;

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
