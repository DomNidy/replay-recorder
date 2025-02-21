#include <iostream>
#include "snapshot_stream.h"

void SnapshotStream::handleDataReceived(const char *data, RecordingType type)
{
    switch (type)
    {
    case RecordingType::AUDIO:
        break;
    case RecordingType::KEYLOG:
        break;
    case RecordingType::SCREENSHOT:
        break;
    case RecordingType::OS_ACTIVITY:
        break;
    }

    std::cout << "data: " << data << std::endl;
}