#ifndef BASE_RECORDER_H
#define BASE_RECORDER_H

#include "snapshot_stream.h"

class BaseRecorder
{
public:
    BaseRecorder(SnapshotStream &targetStream) : stream(targetStream) {}

    /**
     * Monitor for data here
     */
    virtual void writeToStream(const char *data)
    {
        stream.handleDataReceived(data, RecordingType::KEYLOG);
    };

private:
    /**
     * Stream that we write to
     */
    SnapshotStream &stream;
};

#endif