#pragma once

#include <ctime>
#include <string>

namespace RP::Utils
{
/**
 * Converts a numeric day (1-31) to its ordinal representation (e.g., "first", "twenty-second")
 * @param day The day of the month (1-31)
 * @return The ordinal representation of the day as a string
 */
std::string formatTimestampGetOrdinalDay(int day);

/**
 * Formats a timestamp in a human-readable format for LLM consumption
 * Example: "2023 January twenty-first 14:30"
 * @param time Pointer to a tm struct containing the time information
 * @return Formatted timestamp string
 */
std::string formatTimestampToLLMReadable(std::tm *time);
} // namespace RP::Utils