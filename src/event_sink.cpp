#include <iostream>
#include "event_sink.h"

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

void EventSink::addSource(EventSource &source)
{
    std::cout << "EventSink - Added an EventSource!" << std::endl;
    source.initializeSource(this);
}

EventSink &EventSink::operator<<(const char *data)
{
    // Get buffer size needed to perform the conversion
    int len = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);

    std::unique_ptr<wchar_t[]> output = std::make_unique<wchar_t[]>(len);

    // CAUTION: Can cause a buffer overrun because the size of the input buffer (lpMultiByteStr) equals the number of bytes in the string,
    // while the size of the output buffer (lpWideCharStr) equals the number of characters. (But we should be fine since we're converting
    // 1 byte chars to 2 byte chars, i.e., the output should have same or less characters.)
    // NOTE: cbMultiByte can be set to -1 since our string(s) are null terminated (i think)
    // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
    int convertResult = MultiByteToWideChar(CP_UTF8, 0, data, -1, output.get(), len);

    recordingBuffer.insert(recordingBuffer.end(), output.get(), (output.get() + len - 1));
    _flushIfMaxSizeExceeded();
    return *this;
}

EventSink &EventSink::operator<<(const wchar_t *data)
{
    size_t len = 0;
    while (data[len] != '\0')
    {
        len++;
    }

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
        std::string text = converter.to_bytes(recordingBuffer.data(), (recordingBuffer.data() + recordingBuffer.size()));

        file.write(text.data(), text.size());

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