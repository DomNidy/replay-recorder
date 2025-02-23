#ifndef KEYSTROKE_RECORDER_H
#define KEYSTROKE_RECORDER_H

#include "snapshot_stream.h"
#include "recorder.h"

#ifdef _WIN32
#include <windows.h>
#endif

class KeystrokeRecorder : Recorder
{
public:
    KeystrokeRecorder() {};
    ~KeystrokeRecorder() { unregisterRecorder(); }

    virtual void registerRecorder(std::shared_ptr<SnapshotStream> targetStream) override;
    virtual void unregisterRecorder() override;

private:
    virtual void publishData(void *data) override;

private:
    /**
     * Stream that we write to
     */
    static std::shared_ptr<SnapshotStream> stream;

#ifdef _WIN32
private:
    // Store the handle to the keyboard input hook
    static HHOOK hKeyboardHook;

    // Hook procedure that
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif
};

#endif