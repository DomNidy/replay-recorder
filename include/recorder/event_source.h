#pragma once

#include <iostream>
#include <memory>
#include "utils/error_messages.h"

class EventSink;
class EventSource
{
    friend EventSink;

  public:
    EventSource()
    {
    }
    ~EventSource() = default;

    EventSource(const EventSource&) = delete;
    EventSource(EventSource&&) = delete;
    EventSource& operator=(const EventSource&) = delete;
    EventSource& operator=(EventSource&&) = delete;

  public:
    // Initialize the source. The passed EventSink can be used to send events to the source
    virtual void initializeSource(std::shared_ptr<EventSink> inSink) = 0;
};