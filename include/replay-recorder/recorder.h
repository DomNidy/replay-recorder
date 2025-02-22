#ifndef RECORDER_H
#define RECORDER_H

#include <memory>

class Recorder
{
public:
    /**
     * Set up association to the SnapshotStream and register any
     * necessary event listeners/do setup here.
     */
    virtual void registerRecorder(std::shared_ptr<SnapshotStream> targetStream) = 0;

    /**
     * The inverse of registerRecorder(). Removes any event listeners, etc.
     */
    virtual void unregisterRecorder() = 0;

private:
    virtual void publishData(void *data) = 0;
};

#endif