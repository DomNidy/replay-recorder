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

class EventSink : public std::enable_shared_from_this<EventSink>
{
  public:
    EventSink(const std::string& name);
    ~EventSink();

    EventSink& operator<<(const char* data);
    EventSink& operator<<(const wchar_t* data);

  private:
    // Checks buffer size and calls flushData() it if its too big
    inline void flushIfMaxSizeExceeded();

    // Flush all buffered data to file/output stream
    inline void flushData();

    // Vector of buffered data
    std::vector<wchar_t> recordingBuffer;

    // File that we're serializing user activity related events to
    std::ofstream file;
};
