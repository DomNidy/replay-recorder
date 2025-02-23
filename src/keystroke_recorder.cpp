#include "keystroke_recorder.h"

#ifdef _WIN32

HHOOK KeystrokeRecorder::hKeyboardHook = NULL;

LRESULT CALLBACK KeystrokeRecorder::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        // lParam is a pointer to a KBDLLHOOKSTRUCT
        KBDLLHOOKSTRUCT *pKeyboard = (KBDLLHOOKSTRUCT *)lParam;

        switch (wParam)
        {
        case WM_KEYDOWN:
            wchar_t unicodeBuffer[2] = {0};
            static BYTE keyboardState[256] = {0};
            GetKeyboardState(keyboardState);

            // Check if shift key pressed
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            {
                keyboardState[VK_SHIFT] |= 0x80;
            }

            if (ToUnicode(pKeyboard->vkCode, pKeyboard->scanCode, keyboardState, unicodeBuffer, 2, 0) == 1)
            {
                // stream->file.write(static_cast<char *>(unicodeBuffer), 2);


                std::wcout << L"Character typed: " << unicodeBuffer[0] << std::endl;
            }

            break;
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}
#endif

void KeystrokeRecorder::registerRecorder(std::shared_ptr<SnapshotStream> targetStream)
{
    if (!targetStream)
    {
        throw std::runtime_error("Target stream invalid!");
    }

    this->stream = targetStream;

#ifdef _WIN32
    std::cout << "Registering Windows keyboard hook..." << std::endl;
    hKeyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        KeyboardProc,
        GetModuleHandle(NULL),
        0);
    std::cout << "Registered hook: " << hKeyboardHook << std::endl;
#endif
}

void KeystrokeRecorder::unregisterRecorder()
{
#ifdef _WIN32
    if (hKeyboardHook)
    {
        UnhookWindowsHookEx(hKeyboardHook);
        hKeyboardHook = NULL;
        std::cout << "Unregistered Windows keyboard hook" << std::endl;
    }
    else
    {
        std::cout << "Attempted to unregister Windows keyboard hook, but the hook was already null" << std::endl;
    }
#endif
}

void KeystrokeRecorder::publishData(void *data)
{
}
