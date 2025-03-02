#pragma once

#include <fstream>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <assert.h>
#include <string>
#include <cerrno>
#include <memory>
#include <codecvt>
#include <locale>
#include <Windows.h>
#include "event_source.h"

// Cause buffer recording buffer to flush & write to file upon reaching this size
constexpr size_t MAX_RECORDING_BUFFER_SIZE = 10000;

class EventSink
{
public:
    EventSink(const std::string &name);
    ~EventSink();

    EventSink &operator<<(const char *data);
    EventSink &operator<<(const wchar_t *data);

    // Add a source to receive events from (e.g., user input events)
    template <typename T>
    void addSource(EventSource<T> &source);

private:
    // Checks buffer size and calls _flushData() it if its too big
    inline void _flushIfMaxSizeExceeded();

    // Flush all buffered data to file/output stream
    inline void _flushData();

    // Vector of buffered data
    std::vector<wchar_t> recordingBuffer;

    // File that we're serializing events to
    std::ofstream file;
};

template <typename T>
void EventSink::addSource(EventSource<T> &source)
{
    source.initializeSource(this);
}
