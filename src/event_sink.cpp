#include <iostream>
#include "event_sink.h"

// Utility functions
inline bool IsHighSurrogate(wchar_t c) { return (c >= 0xD800 && c <= 0xDBFF); }
inline bool IsLowSurrogate(wchar_t c) { return (c >= 0xDC00 && c <= 0xDFFF); }

EventSink::EventSink(const std::string &name)
{
    file.open(name, std::ios::out | std::ios::app | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open output file for EventSink - " + name + ", " + std::strerror(errno));
    }
}

EventSink::~EventSink()
{
    if (file.is_open())
    {
        _flushData();
        file.close();
    }
}

EventSink &EventSink::operator<<(const char *data)
{
    // Get buffer size needed to perform the conversion
    int len = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
    if (len == 0)
    {
        std::cerr << "Failed to calculate buffer size for UTF-8 to UTF-16 conversion: " << GetLastError() << "\n";
        return *this;
    }

    std::unique_ptr<wchar_t[]> output = std::make_unique<wchar_t[]>(len);
    int convertResult = MultiByteToWideChar(CP_UTF8, 0, data, -1, output.get(), len); // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
    if (convertResult == 0)
    {
        std::cerr << "UTF-8 to UTF-16 conversion failed: " << GetLastError() << "\n";
        return *this;
    }

    recordingBuffer.insert(recordingBuffer.end(), output.get(), output.get() + len - 1);
    _flushIfMaxSizeExceeded();
    return *this;
}

EventSink &EventSink::operator<<(const wchar_t *data)
{
    size_t len = wcslen(data);

    recordingBuffer.insert(recordingBuffer.end(), data, (data + len));
    _flushIfMaxSizeExceeded();
    return *this;
}

inline void EventSink::_flushData()
{
    if (!recordingBuffer.empty())
    {
        std::cout << "Flushing recording buffer & writing to file\n";
        std::cout << "Recording buffer size: " << recordingBuffer.size();

        // Convert recording buffer's wchar_t array to an std::string so we can save to UTF-8
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

        try
        {
            std::string text = converter.to_bytes(recordingBuffer.data(), (recordingBuffer.data() + recordingBuffer.size()));
            file.write(text.data(), text.size());
        }
        catch (const std::range_error &e)
        {
            std::cerr << "Error converting UTF-16 to UTF-8: " << e.what() << "\n";
        }

        file.flush();
        recordingBuffer.clear();
    }
}

inline void EventSink::_flushIfMaxSizeExceeded()
{
    if (recordingBuffer.size() >= MAX_RECORDING_BUFFER_SIZE)
    {
        _flushData();
    }
}