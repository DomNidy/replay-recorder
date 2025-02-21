#include <iostream>
#include "snapshot_stream.h"
#include "base_recorder.h"

int main()
{

    SnapshotStream stream("out.txt");

    BaseRecorder rec1(stream);

    rec1.writeToStream("Hello world123!");
}