#include "keystroke_recorder.h"

void KeystrokeRecorder::registerRecorder(std::shared_ptr<SnapshotStream> targetStream)
{
    if (!targetStream)
    {
        throw std::runtime_error("Target stream invalid!");
    }

    this->stream = targetStream;

#ifdef _WIN32
    std::cout << "registerRecorder() ran & we're running the Windows binary" << std::endl;
#endif
}

void KeystrokeRecorder::unregisterRecorder()
{
#ifdef _WIN32
    std::cout << "unreigsterRecorder() ran & we're running the Windows binary" << std::endl;
#endif
}

void KeystrokeRecorder::publishData(void *data)
{
}
