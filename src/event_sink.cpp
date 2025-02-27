#include <iostream>
#include "event_sink.h"

EventSink::EventSink(const std::string &name)
{
    file.open(name, std::ios::out | std::ios::app);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open output file for EventSink - " + name + ", " + std::strerror(errno));
    }
}

EventSink::~EventSink()
{
    if (file.is_open())
    {
        flushData();
        file.close();
    }
}

void EventSink::addSource(EventSource *source)
{
    std::cout << "EventSink - Added an EventSource!" << std::endl;
    source->initializeSource(this);
}

void EventSink::flushData()
{
    if (!recordingBuffer.empty())
    {
        std::cout << "Flushing recording buffer & writing to file\n";
        std::cout << "Recording buffer size: " << recordingBuffer.size();

        file.write(recordingBuffer.data(), recordingBuffer.size());
        file.flush();
        recordingBuffer.clear();
    }
}