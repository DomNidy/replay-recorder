#pragma once

#include <windows.h>

#include <assert.h>
#include <cerrno>
#include <codecvt>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <locale>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "event_source.h"

// Cause buffer recording buffer to flush & write to file upon reaching this
// size
constexpr size_t MAX_RECORDING_BUFFER_SIZE = 1500;

class EventSink
{
  public:
    EventSink(const std::string &name);
    ~EventSink();

    EventSink &operator<<(const char *data);
    EventSink &operator<<(const wchar_t *data);

    // Add a source to receive events from (e.g., user input events)
    template <typename T> void addSource(EventSource<T> &source);

  private:
    // Checks buffer size and calls _flushData() it if its too big
    inline void _flushIfMaxSizeExceeded();

    // Flush all buffered data to file/output stream
    inline void _flushData();

    // Vector of buffered data
    std::vector<wchar_t> recordingBuffer;

    // File that we're serializing user activity related events to
    std::ofstream file;
};

template <typename T> void EventSink::addSource(EventSource<T> &source)
{
    source.initializeSource(this);
}
