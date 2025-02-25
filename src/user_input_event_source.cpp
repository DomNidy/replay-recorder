#include "user_input_event_source.h"
#include <string.h>
#include <assert.h>
#include <TlHelp32.h>

/** Static variable initialization */
UserInputEventSource *UserInputEventSource::instance = nullptr;
HHOOK UserInputEventSource::hKeyboardHook = NULL;
EventSink *UserInputEventSource::outputSink = nullptr;

UserInputEventSource *UserInputEventSource::getInstance()
{
    if (instance == nullptr)
    {
        instance = new UserInputEventSource();
    }
    return instance;
}

LRESULT CALLBACK UserInputEventSource::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        // lParam is a pointer to a KBDLLHOOKSTRUCT
        KBDLLHOOKSTRUCT *pKeyboard = (KBDLLHOOKSTRUCT *)lParam;

        switch (wParam)
        {
        case WM_SYSKEYDOWN:
            // I think we need to check for ALT separately in here. Alt is sys key.
        case WM_KEYDOWN:
            wchar_t unicodeBuffer[2] = {0};
            static BYTE keyboardState[256] = {0};
            GetKeyboardState(keyboardState);

            // Check if shift key pressed
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            {
                std::cout << "[SHIFT]" << std::endl;
                keyboardState[VK_SHIFT] |= 0x80;
            }

            // Check if caps lock is on
            if (GetKeyState(VK_CAPITAL) & 0x0001)
            {
                std::cout << "[CAPS LOCK]" << std::endl;
                keyboardState[VK_CAPITAL] |= 0x01;
            }

            // Check if ctrl key pressed
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            {
                std::cout << "[CTRL]" << std::endl;
                keyboardState[VK_CONTROL] |= 0x80;
            }

            // Convert the input to unicode character (need to do this since the input can change based on kbd state
            // and the keyboard layout)
            if (ToUnicode(pKeyboard->vkCode, pKeyboard->scanCode, keyboardState, unicodeBuffer, 2, 0) == 1)
            {
                // Send data to output sink
                *outputSink << unicodeBuffer;
            }

            break;
        }
    }

    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void UserInputEventSource::initializeSource(EventSink *inSink)
{
    outputSink = inSink;
    assert(outputSink != nullptr);

    // We need to get a handle to the module (a loaded .dll or .exe) that contains the hook procedure code
    // Since the hook proc code will exist in the same binary file, we set the module name to null here
    // which returns a handle to the file used to create the calling process (calling process is our app)
    HMODULE hMod = GetModuleHandle(NULL);

    // Install the hook
    hKeyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        KeyboardProc,
        hMod,
        0);

    if (!hKeyboardHook)
    {
        throw std::runtime_error("Failed to register keyboard hook: " + std::to_string(GetLastError()));
    }

    std::cout << "UserInputEventSource successfully installed keyboard hook" << std::endl;
}

void UserInputEventSource::uninitializeSource()
{
    assert(hKeyboardHook != nullptr);

    UnhookWindowsHookEx(hKeyboardHook);
    hKeyboardHook = NULL;
    std::cout << "UserInputEventSource successfully uninstalled keyboard hook" << std::endl;
}
