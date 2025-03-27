#pragma once

#include <string>

namespace RP::ErrorMessages
{
    // Windows Hook Manager related errors
    inline const std::string OBSERVER_UNREGISTER_FAILED = 
        "Could not unregister observer from WindowsHookManager - object no longer owned by shared_ptr. "
        "This happens when the destructors for WindowsHookManager and the ObserverClassData run first "
        "and end up decrementing the ref count to 0. This is weird I should re-evaluate my usage of "
        "unregisterObserver(). Warning so I remember to fix this.";

    // Event Source related errors
    inline const std::string INITIALIZED_WITH_NULLPTR_EVENT_SINK = 
        "initializeSource() was called with inSink == nullptr, Need an EventSink to initialize a source!";
} 