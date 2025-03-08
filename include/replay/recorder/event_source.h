#pragma once

#include <iostream>

template <typename T>
class EventSource {
  friend class EventSink;

 public:
  static T &getInstance() {
    static T instance;
    return instance;
  }

  EventSource() { std::cout << "EVENT SOURCE CONSTRUCTOR CALLED\n"; }
  ~EventSource() = default;

  EventSource(const EventSource &) = delete;
  EventSource(EventSource &&) = delete;
  EventSource &operator=(const EventSource &) = delete;
  EventSource &operator=(EventSource &&) = delete;

 private:
  friend class EventSink;

  /**
   * Called by EventSink to add an EventSource
   */
  virtual void initializeSource(class EventSink *inSink) = 0;
  virtual void uninitializeSource() = 0;
};