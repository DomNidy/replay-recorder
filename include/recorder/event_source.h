#pragma once

#include <iostream>
#include <memory>

#define RP_ERR_INITIALIZED_WITH_NULLPTR_EVENT_SINK                                                                     \
    "initializeSource() was called with inSink == nullptr, Need an EventSink to initialize a source!"

class EventSink;
class EventSource
{
    friend EventSink;

  public:
    EventSource()
    {
    }
    ~EventSource() = default;

    EventSource(const EventSource &) = delete;
    EventSource(EventSource &&) = delete;
    EventSource &operator=(const EventSource &) = delete;
    EventSource &operator=(EventSource &&) = delete;

  private:
    /**
     * Called by EventSink to add an EventSource
     */
    virtual void initializeSource(std::shared_ptr<EventSink> inSink) = 0;
};