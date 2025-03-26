#pragma once

#include <Windows.h>
#include <iostream>
#include <memory>
#include <queue>
#include <typeindex>
#include <vector>
#include "utils/logging.h"

//-------------------------------------------------------
// Hook types - Identifiers for different windows hooks
//-------------------------------------------------------
namespace Replay::Windows
{

enum class HookType
{
    None = 0,
    LowLevelKeyboard = 1 << 0,      // WH_KEYBOARD_LL: Notified about all keyboard events
    EventSystemForeground = 1 << 1, // EVENT_SYSTEM_FOREGROUND: Notified when the
                                    // foreground window changes
};
using HookTypeFlags = unsigned int;

namespace internal
{
// Operator overloading for bitwise operations on HookType
inline HookType operator|(HookType a, HookType b)
{
    return static_cast<HookType>(static_cast<int>(a) | static_cast<int>(b));
}

inline HookType operator&(HookType a, HookType b)
{
    return static_cast<HookType>(static_cast<int>(a) & static_cast<int>(b));
}

} // namespace internal

//-------------------------------------------------------
// Observer Interfaces
//-------------------------------------------------------
template <typename... CallbackArgs>
class BaseObserver
{
  protected:
    // Helper method to convert HookType enum to its internal bit flag
    static HookTypeFlags hookTypeToFlag(HookType type)
    {
        return static_cast<HookTypeFlags>(type);
    }

  public:
    virtual ~BaseObserver() = default;

    // Observer implementations should override this with the hooks they need
    // never call this method directly
    virtual HookTypeFlags getRequiredHooks()
    {
        // warn that this is not overridden
        std::cerr << "Warning: getRequiredHooks is not overridden in the concrete "
                     "observer class"
                  << std::endl;
        return hookTypeToFlag(HookType::None);
    }

  public:
    bool requiresHook(HookType hook)
    {
        return getRequiredHooks() & hookTypeToFlag(hook);
    }

  protected:
    BaseObserver() = default;

  private:
    BaseObserver(const BaseObserver&) = delete;
    BaseObserver& operator=(const BaseObserver&) = delete;
};

// Data the KeyboardInputObserver receives from windows KeyboardProc
struct KeyboardInputEventData
{
    int nCode;
    WPARAM wParam;
    LPARAM lParam;

  private:
    KeyboardInputEventData();
    KeyboardInputEventData(int code, WPARAM wp, LPARAM lp);

    // Only make the constructors visible to the WindowsHookManager class
    friend class WindowsHookManager;
};

// Observer interface for keyboard input events
class KeyboardInputObserver : public BaseObserver<KeyboardInputEventData>
{

  public:
    HookTypeFlags getRequiredHooks() override
    {
        return hookTypeToFlag(HookType::LowLevelKeyboard);
    }
    virtual void onKeyboardInput(KeyboardInputEventData eventData) = 0;
    virtual ~KeyboardInputObserver() = default;
};

// Observer interface for focus change events
class FocusObserver : public BaseObserver<HWND>
{
  public:
    HookTypeFlags getRequiredHooks() override
    {
        return hookTypeToFlag(HookType::EventSystemForeground);
    }
    virtual void onFocusChange(HWND hwnd) = 0;
    virtual ~FocusObserver() = default;
};

class WindowsHookManager
{
  private:
    // Private constructor for singleton pattern
    WindowsHookManager()
    {
    }

    // This exists to allow polymorphic storage of ObserverClassData
    class BaseObserverClassData
    {
      public:
        virtual ~BaseObserverClassData() = default;
    };

    // Class-level data for each ConcreteObserver class (not instance, based on type)
    // ConcreteObserver is the type of the observer class
    // ConcreteObserver receives EventData... in its callback
    template <typename ConcreteObserver, typename... EventData>
    struct ObserverClassData : public BaseObserverClassData
    {
        // TODO: We should prob make destructor join this thread
        ObserverClassData()
        {
            eventLoopThread = std::thread(&ObserverClassData::eventLoopThreadFunc, this);
        }
        // Queue of events for this observer class
        std::queue<std::tuple<EventData...>> eventQueue;
        std::mutex eventQueueMutex;

        // List of observers for this observer class
        std::vector<std::shared_ptr<ConcreteObserver>> observers;
        std::mutex observersMutex;

        // The thread which is waiting for events and calling the observer's
        // callback
        std::thread eventLoopThread;

        // Notify this to wake up the event loop thread (do when events are added)
        std::condition_variable eventQueueConditionVariable;

      private:
        // Wait for events to be added to queue, process them, then go back to sleep
        void eventLoopThreadFunc()
        {
            while (true)
            {
                std::unique_lock<std::mutex> lock(eventQueueMutex);

                // Sleep thread and only awake when we're notified and there are events to process
                eventQueueConditionVariable.wait(lock, [this]() { return !eventQueue.empty(); });

                // Process all events in the queue
                while (!eventQueue.empty())
                {
                    // Get the event from the queue
                    auto eventTuple = eventQueue.front();
                    eventQueue.pop();
                    lock.unlock();

                    // Process all observers (with lock for thread safety)
                    std::lock_guard<std::mutex> observersLock(observersMutex);
                    for (auto& observer : observers)
                    {
                        // Notify each observer with the event data
                        std::apply(
                            [&observer](const auto&... args) {
                                if constexpr (std::is_same_v<ConcreteObserver, KeyboardInputObserver>)
                                {
                                    observer->onKeyboardInput(args...);
                                }
                                else if constexpr (std::is_same_v<ConcreteObserver, FocusObserver>)
                                {
                                    observer->onFocusChange(args...);
                                }
                            },
                            eventTuple);
                    }

                    // Re-acquire the lock for the next iteration
                    lock.lock();
                }
            }
        }
    };

    // Map observer class type to its event queue
    std::unordered_map<std::type_index, std::shared_ptr<BaseObserverClassData>> observerClassData;
    std::mutex observerClassDataMutex;

    // Map of hook types and the corresponding unhook function
    using UnhookFunctionPtr = std::function<void()>;
    std::map<HookType, UnhookFunctionPtr> unhookFunctions;

  private:
    // Deleted copy constructor and assignment operator
    WindowsHookManager(const WindowsHookManager&) = delete;
    WindowsHookManager& operator=(const WindowsHookManager&) = delete;

    // Static hook procedures
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                                      DWORD idEventThread, DWORD dwmsEventTime);

  public:
    // Meyer's singleton pattern
    static WindowsHookManager& getInstance()
    {
        static WindowsHookManager instance;
        return instance;
    }

    // Destructor
    ~WindowsHookManager()
    {
        uninstallHooks();
    }

  private:
    // Internal method that installs hooks for a given observer if they are not
    // already installed. Observers need to receive data from windows hooks.
    template <typename ObserverType>
    void internalInstallHooksForObserver(ObserverType* observer)
    {
        // Low level keyboard hook installation
        if (observer->requiresHook(HookType::LowLevelKeyboard) &&
            unhookFunctions.find(HookType::LowLevelKeyboard) == unhookFunctions.end())
        {
            // Install the hook
            HHOOK hookHandle = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);

            if (!hookHandle)
            {
                throw std::runtime_error("Failed to install keyboard hook");
            }

            // Create a lambda function which can unhook the keyboard hook
            unhookFunctions[HookType::LowLevelKeyboard] = [hookHandle]() { UnhookWindowsHookEx(hookHandle); };

            LOG_CLASS_INFO("WindowsHookManager", "Keyboard hook installed successfully");
        }
        // Event system foreground hook installation
        if (observer->requiresHook(HookType::EventSystemForeground) &&
            unhookFunctions.find(HookType::EventSystemForeground) == unhookFunctions.end())
        {

            // Install the hook
            HWINEVENTHOOK hookHandle = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL,
                                                       WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);

            if (!hookHandle)
            {
                LOG_CLASS_ERROR("WindowsHookManager", "Failed to install focus hook");
                throw std::runtime_error("Failed to install focus hook");
            }

            // Create a lambda function which can unhook the focus hook
            unhookFunctions[HookType::EventSystemForeground] = [hookHandle]() { UnhookWinEvent(hookHandle); };

            LOG_CLASS_INFO("WindowsHookManager", "Focus hook installed successfully");
        }
    }

    // Create a class data entry for a given observer class type
    template <typename ConcreteObserver, typename... EventData>
    std::shared_ptr<ObserverClassData<ConcreteObserver, EventData...>> internalCreateClassDataEntry()
    {
        assert(observerClassData.find(typeid(ConcreteObserver)) == observerClassData.end() &&
               "Class data entry already exists for type");

        {
            std::lock_guard<std::mutex> lock(observerClassDataMutex);

            // Create class data entry and assign it to the observerClassData map
            auto sharedClassData = std::make_shared<ObserverClassData<ConcreteObserver, EventData...>>();
            observerClassData[typeid(ConcreteObserver)] = sharedClassData;

            LOG_CLASS_INFO("WindowsHookManager", "Class data entry created for type {}",
                           typeid(ConcreteObserver).name());

            // Return the shared pointer directly
            return sharedClassData;
        }

        return nullptr;
    }

    // Get pointer to the class data entry for the given observer class type
    // Template is used so the returned struct will be typed correctly
    template <typename ConcreteObserver, typename... EventData>
    ObserverClassData<ConcreteObserver, EventData...>* getObserverClassDataEntry()
    {
        auto it = observerClassData.find(typeid(ConcreteObserver));
        if (it != observerClassData.end())
        {
            return dynamic_cast<ObserverClassData<ConcreteObserver, EventData...>*>(it->second.get());
        }
        return nullptr;
    }

  public:
    // Register keyboard observers
    template <typename ConcreteObserver>
    void registerObserver(std::shared_ptr<ConcreteObserver> observer)
    {
        internalInstallHooksForObserver(observer.get());

        if constexpr (std::is_base_of_v<KeyboardInputObserver, ConcreteObserver>)
        {
            // Create the class data entry for the keyboard observer if it doesn't exist
            if (getObserverClassDataEntry<KeyboardInputObserver, KeyboardInputEventData>() == nullptr)
            {
                auto classData = internalCreateClassDataEntry<KeyboardInputObserver, KeyboardInputEventData>();
            }

            // Get the class data entry for the keyboard observer
            auto keyboardObserverClassData = getObserverClassDataEntry<KeyboardInputObserver, KeyboardInputEventData>();
            assert(keyboardObserverClassData != nullptr);

            // Add the observer to the list of keyboard observers, locking it beforehand
            {
                std::lock_guard<std::mutex> lock(keyboardObserverClassData->observersMutex);
                keyboardObserverClassData->observers.push_back(
                    std::static_pointer_cast<KeyboardInputObserver>(observer));
            }
        }
        else if constexpr (std::is_base_of_v<FocusObserver, ConcreteObserver>)
        {
            if (getObserverClassDataEntry<FocusObserver, HWND>() == nullptr)
            {
                internalCreateClassDataEntry<FocusObserver, HWND>();
            }

            auto focusObserverClassData = getObserverClassDataEntry<FocusObserver, HWND>();
            assert(focusObserverClassData != nullptr);

            {
                std::lock_guard<std::mutex> lock(focusObserverClassData->observersMutex);
                focusObserverClassData->observers.push_back(std::static_pointer_cast<FocusObserver>(observer));
            }
        }
    }

    template <typename ConcreteObserver>
    void unregisterObserver(std::shared_ptr<ConcreteObserver> observer)
    {
        if constexpr (std::is_base_of_v<KeyboardInputObserver, ConcreteObserver>)
        {
            std::lock_guard<std::mutex> lock(observerClassDataMutex);

            auto keyboardObserverClassData = getObserverClassDataEntry<KeyboardInputObserver, KeyboardInputEventData>();
            assert(keyboardObserverClassData != nullptr);

            // Remove the observer instance from the its class data entry
            {
                std::lock_guard<std::mutex> lock(keyboardObserverClassData->observersMutex);

                // Find the observer instance by looking if they point to same location
                auto it = std::find_if(
                    keyboardObserverClassData->observers.begin(), keyboardObserverClassData->observers.end(),
                    [observer](std::shared_ptr<KeyboardInputObserver> obs) { return observer.get() == obs.get(); });

                if (it != keyboardObserverClassData->observers.end())
                {
                    keyboardObserverClassData->observers.erase(it);
                }
                else
                {
                    LOG_CLASS_WARN("WindowsHookManager",
                                   "Tried to unregister keyboard observer that was not found in class data entry");
                }
            }
        }
        else if constexpr (std::is_base_of_v<FocusObserver, ConcreteObserver>)
        {
            std::lock_guard<std::mutex> lock(observerClassDataMutex);

            auto focusObserverClassData = getObserverClassDataEntry<FocusObserver, HWND>();
            assert(focusObserverClassData != nullptr);

            {
                std::lock_guard<std::mutex> lock(focusObserverClassData->observersMutex);

                auto it = std::find_if(
                    focusObserverClassData->observers.begin(), focusObserverClassData->observers.end(),
                    [observer](std::shared_ptr<FocusObserver> obs) { return observer.get() == obs.get(); });

                if (it != focusObserverClassData->observers.end())
                {
                    focusObserverClassData->observers.erase(it);
                }
                else
                {
                    LOG_CLASS_WARN("WindowsHookManager",
                                   "Tried to unregister focus observer that was not found in class data entry");
                }
            }
        }
    }

    void uninstallHooks()
    {
        std::vector<HookType> hooksToUninstall;
        for (const auto& [hookType, _] : unhookFunctions)
        {
            hooksToUninstall.push_back(hookType);
        }

        if (hooksToUninstall.empty())
        {
            LOG_CLASS_INFO("WindowsHookManager", "No hooks to uninstall");
            return;
        }

        for (const auto& hookType : hooksToUninstall)
        {
            unhookFunctions[hookType]();
            LOG_CLASS_INFO("WindowsHookManager", "Hook of type {} uninstalled", static_cast<int>(hookType));
            unhookFunctions.erase(hookType);
        }
    }
};

} // namespace Replay::Windows