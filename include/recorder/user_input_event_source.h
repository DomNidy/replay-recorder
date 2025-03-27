#pragma once

#include <windows.h>

#include <TlHelp32.h>
#include <memory>
#include "windows_hook_manager.h"

#include "event_source.h"

/**
 * Singleton that records user input events (e.g., keyboard)
 */
class UserInputEventSource : public EventSource,
                             public Replay::Windows::KeyboardInputObserver,
                             public std::enable_shared_from_this<UserInputEventSource>
{
  public:
    UserInputEventSource() = default;
    ~UserInputEventSource();

  private:
    //~ Begin EventSource interface
    virtual void initializeSource(std::weak_ptr<EventSink> inSink) override;
    //~ End EventSource interface

    //~ Begin KeyboardInputObserver interface
    virtual void onKeyboardInput(Replay::Windows::KeyboardInputEventData eventData) override;
    //~ End KeyboardInputObserver interface

  private:
    // This state is used to detect ALT+TAB
    bool leftAltPressed = false;
    bool tabPressed = false;

  private:
    // The EventSink that registered us and we should write to
    std::weak_ptr<EventSink> outputSink;

    // Enumerate all processes on the Windows system
    void enumerateWindowsProcesses() const;
};
