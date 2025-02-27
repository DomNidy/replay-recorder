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
#include <assert.h>
#include <string>
#include <cerrno>
#include <Windows.h>
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
        std::wstring wideWhat;

        // Get buffer size needed to perform the conversion
        int len = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
        assert(len > 0);

        wchar_t *output = new wchar_t[len];

        // CAUTION: Can cause a buffer overrun because the size of the input buffer (lpMultiByteStr) equals the number of bytes in the string,
        // while the size of the output buffer (lpWideCharStr) equals the number of characters. (But we should be fine since we're converting
        // 1 byte chars to 2 byte chars, i.e., the output should have same or less characters.)
        // NOTE: cbMultiByte can be set to -1 since our string(s) are null terminated (i think)
        // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
        int convertResult = MultiByteToWideChar(CP_UTF8, 0, data, -1, output, len);
        assert(convertResult > 0);

        recordingBuffer.insert(recordingBuffer.end(), output, (output + len - 1));

        return *this;
    }

    EventSink &operator<<(const wchar_t *data)
    {
        size_t len = 0;
        while (data[len] != '\0')
        {
            len++;
        }

        recordingBuffer.insert(recordingBuffer.end(), data, (data + len));
        return *this;
    }

    // Add a source to receive events from (e.g., user input events)
    void addSource(EventSource *source);

private:
    // Flush all buffered data to file/output stream
    void flushData();

private:
    // Vector of buffered data
    std::vector<wchar_t> recordingBuffer;

    // File that we're serializing events to
    std::wfstream file;
};