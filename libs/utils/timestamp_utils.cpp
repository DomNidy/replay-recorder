#include "timestamp_utils.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace RP::Utils
{
// Replaced switch statement with a static vector lookup for conciseness
std::string formatTimestampGetOrdinalDay(int day)
{
    if (day < 1 || day > 31)
    {
        return ""; // Return empty string for invalid days
    }
    // Static const vector for efficient lookup, initialized once
    // TODO: Should we just use statically sized array or is vector same level perf wise?
    // Maybe compiler optimizes it due to const idk
    static const std::vector<std::string> ordinals = {"", // Index 0 unused
                                                      "first",
                                                      "second",
                                                      "third",
                                                      "fourth",
                                                      "fifth",
                                                      "sixth",
                                                      "seventh",
                                                      "eighth",
                                                      "ninth",
                                                      "tenth",
                                                      "eleventh",
                                                      "twelfth",
                                                      "thirteenth",
                                                      "fourteenth",
                                                      "fifteenth",
                                                      "sixteenth",
                                                      "seventeenth",
                                                      "eighteenth",
                                                      "nineteenth",
                                                      "twentieth",
                                                      "twenty-first",
                                                      "twenty-second",
                                                      "twenty-third",
                                                      "twenty-fourth",
                                                      "twenty-fifth",
                                                      "twenty-sixth",
                                                      "twenty-seventh",
                                                      "twenty-eighth",
                                                      "twenty-ninth",
                                                      "thirtieth",
                                                      "thirty-first"};
    return ordinals[day];
}

std::string formatTimestampToLLMReadable(std::tm* time)
{
    if (!time)
    {
        throw std::runtime_error("Received nullptr instead of std::tm");
    }

    std::string dayOrdinal = formatTimestampGetOrdinalDay(time->tm_mday);
    if (dayOrdinal.empty())
    {
        throw std::runtime_error("Invalid day encountered while formatting timestamp");
    }

    std::stringstream ss;
    ss << std::put_time(time, "%Y %B") << " " << dayOrdinal << std::put_time(time, " %H:%M");

    return ss.str();
}

} // namespace RP::Utils