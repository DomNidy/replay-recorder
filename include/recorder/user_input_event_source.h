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
    // Factory method
    static std::shared_ptr<UserInputEventSource> create();

  protected:
    UserInputEventSource() = default;

  public:
    //~ Begin EventSource interface
    virtual void initializeSource(std::shared_ptr<EventSink> inSink) override;
    //~ End EventSource interface

    ~UserInputEventSource();

  private:
    //~ Begin KeyboardInputObserver interface
    virtual void onKeyboardInput(Replay::Windows::KeyboardInputEventData eventData) override;
    //~ End KeyboardInputObserver interface

  private:
    // This state is used to detect ALT+TAB
    bool leftAltPressed = false;
    bool tabPressed = false;

  private:
    std::shared_ptr<EventSink> outputSink;
};
