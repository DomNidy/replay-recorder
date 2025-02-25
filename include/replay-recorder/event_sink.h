#pragma once

#include <fstream>
#include <string>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <string>
#include <cerrno>
#include "event_source.h"

/**
 * Represents the type of data a byte sequence represents
 */
enum class RecordingType : uint8_t
{
    KEYLOG,
    SCREENSHOT,
    AUDIO,
    OS_ACTIVITY
};

class EventSink
{
public:
    EventSink(const std::string &name);
    ~EventSink();

    EventSink &operator<<(const char *data)
    {
        // TODO: Need to append to buffer
        std::cout << data << std::endl;
        return *this;
    }
    EventSink &operator<<(const wchar_t *data)
    {
        // TODO: Need to append to buffer
        std::wcout << data << std::endl;
        return *this;
    }

    // Add a source to receive events from (e.g., user input events)
    void addSource(EventSource *source);

private:
    // Flush all buffered data to file/output stream
    void flushData();

private:
    // Vector of buffered data
    std::vector<char> recordingBuffer;

    // File that we're serializing events to
    std::fstream file;
};