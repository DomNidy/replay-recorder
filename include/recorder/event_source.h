#pragma once

#include <iostream>
#include <memory>
#include "utils/error_messages.h"

class EventSource
{
    friend class EventSink;

  public:
    virtual ~EventSource() = default;

    EventSource(const EventSource&) = delete;
    EventSource(EventSource&&) = delete;
    EventSource& operator=(const EventSource&) = delete;
    EventSource& operator=(EventSource&&) = delete;

  public:
    // Initialize the source. The passed EventSink can be used to send events to the source
    virtual void initializeSource(std::shared_ptr<EventSink> inSink) = 0;

  protected:
    // Only allow derived classes to call this
    // Dervied EventSources should be created through a create factory method
    explicit EventSource() = default;
};