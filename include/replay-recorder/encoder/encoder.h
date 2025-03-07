#pragma once

#include <iostream>
#include <string>

// @brief Our objective with "encoding" differs from traditional encoding.
// We aim to encode the file such that the resulting file is easier for LLMs to interpret.
//
// Intuitively:
//      The more "unimportant" or "redundant" tokens our activity log contains,
//      the more "noise" an LLM be forced to consider while making token predictions.
//      In other words, the entropy of the softmax probabilities will be slightly higher
//      resulting in more "random" response from models.
namespace RP::Encoder
{
    /// @brief Performs RLE on a user activity string, focusing on reducing special token count (e.g., '[LSHIFT]')
    /// @param recordingString String produced by a recording. Contains all of the user activity related events.
    /// @return A RLE'd version of the string that is smaller.
    std::string rle(const std::string &userActivityString);
}