#pragma once

#include <windows.h>

#include <string>

#include "event_source.h"
#include "windows_hook_manager.h"

// Tokens to identify window change events in the event stream
constexpr const char* WINDOW_CHANGE_TOKEN = "[CHANGE_WINDOW]";
constexpr const char* WINDOW_CHANGE_END_TOKEN = "[/CHANGE_WINDOW]";

class UserWindowActivityEventSource : public EventSource,
                                      public Replay::Windows::FocusObserver,
                                      public std::enable_shared_from_this<UserWindowActivityEventSource>
{
  public:
    UserWindowActivityEventSource() = default;
    ~UserWindowActivityEventSource();

    //~ Begin EventSource interface
    virtual void initializeSource(std::shared_ptr<EventSink> inSink) override;
    //~ End EventSource interface

  private:
    //~ Begin Replay::Windows::FocusObserver interface
    void onFocusChange(HWND hwnd) override;
    //~ End Replay::Windows::FocusObserver interface

  private:
    std::shared_ptr<EventSink> outputSink;
    bool getWindowTitle(HWND hWindow, std::string& destStr);
};