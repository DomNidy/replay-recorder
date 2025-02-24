#ifndef KEYSTROKE_RECORDER_H
#define KEYSTROKE_RECORDER_H

#include "snapshot_stream.h"
#include "recorder.h"

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Singleton that listens for keyboard input events
 */
class KeystrokeRecorder : Recorder
{

private:
    /** Implementing singleton */
    KeystrokeRecorder() {};
    ~KeystrokeRecorder() { unregisterRecorder(); }
    static KeystrokeRecorder *instance;

    /** Deleting copy constructor & assignment operator */
    KeystrokeRecorder(const KeystrokeRecorder &) = delete;
    KeystrokeRecorder &operator=(const KeystrokeRecorder &) = delete;

public:
    static KeystrokeRecorder *getInstance()
    {
        if (instance == nullptr)
        {
            instance = new KeystrokeRecorder();
        }
        return instance;
    }

public:
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
private:
    // Store the handle to the keyboard input hook
    static HHOOK hKeyboardHook;

    // Hook procedure that
    static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

    // Enumerate all processes on the Windows system
    void enumerateWindowsProcesses() const;
#endif
};

#endif