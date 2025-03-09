#include <windows.h>

#include <csignal>
#include <iostream>

#include "event_sink.h"
#include "user_input_event_source.h"
#include "user_window_activity_event_source.h"

EventSink *g_eventSink = nullptr;

// Flush the event sink & serialize it to file
void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        std::cout << "Ctrl+C detected. Performing cleanup..." << std::endl;

        if (g_eventSink)
        {
            // Destructor makes sure data is flushed & written to file
            delete g_eventSink;
            g_eventSink = nullptr;
        }

        // Exit the program gracefully
        exit(0);
    }
}

int main(int argc, char **argv)
{
    std::signal(SIGINT, signalHandler);

    // Create EventSink (receives events pertaining to the user's activity)
    EventSink *eventSink = new EventSink("out.txt");
    g_eventSink = eventSink;

    // Create EventSources to monitor user activity
    UserInputEventSource &inputEventSource = UserInputEventSource::getInstance();
    UserWindowActivityEventSource &windowActivityEventSource = UserWindowActivityEventSource::getInstance();

    // Add sources to sink
    eventSink->addSource(inputEventSource);
    eventSink->addSource(windowActivityEventSource);

    BOOL ret;
    MSG msg;
    while ((ret = GetMessage(&msg, NULL, WM_KEYFIRST, WM_MOUSELAST)) != 0)
    {
        if (ret == -1)
        {
            throw std::runtime_error("Error retrieving message with GetMessage(), hWnd is probably an "
                                     "invalid window handle, or lpMsg is an invalid pointer! Error code:" +
                                     GetLastError());
        }
    }
}