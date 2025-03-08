#pragma once

#include <windows.h>

#include <string>

#include "event_source.h"

class UserWindowActivityEventSource
    : public EventSource<UserWindowActivityEventSource> {
  friend class EventSource<UserWindowActivityEventSource>;

 private:
  UserWindowActivityEventSource() = default;
  ~UserWindowActivityEventSource() { uninitializeSource(); }

 private:
  virtual void initializeSource(class EventSink *inSink) override;
  virtual void uninitializeSource() override;

  static EventSink *outputSink;

  // Returns the name of the process corresponding to the focused window
  bool getWindowTitle(HWND hWindow, std::string &destStr);

  // Windows event callback
  static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event,
                                    HWND hWnd, LONG idObject, LONG idChild,
                                    DWORD dwEventThread, DWORD dwmsEventTime);
  HWINEVENTHOOK hWinEventHook;
};