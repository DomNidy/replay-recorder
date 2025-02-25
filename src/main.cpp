#include <iostream>
#include "user_input_event_source.h"
#include "event_sink.h"
#include "WinUser.h"

int main()
{
    EventSink eventSink = EventSink("out.txt");

    UserInputEventSource *keystrokeRecorder = UserInputEventSource::getInstance();

    eventSink.addSource(keystrokeRecorder);

    BOOL ret;
    MSG msg;
    while ((ret = GetMessage(&msg, NULL, WM_KEYFIRST, WM_MOUSELAST)) != 0)
    {
        if (ret == -1)
        {
            throw std::runtime_error("Error retrieving message with GetMessage(), hWnd is probably an invalid window handle, or lpMsg is an invalid pointer! Error code:" + GetLastError());
        }
    }
}