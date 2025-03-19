#include "timestamp_utils.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace RP::Utils
{
std::string formatTimestampGetOrdinalDay(int day)
{
    switch (day)
    {
    case 1:
        return "first";
    case 2:
        return "second";
    case 3:
        return "third";
    case 4:
        return "fourth";
    case 5:
        return "fifth";
    case 6:
        return "sixth";
    case 7:
        return "seventh";
    case 8:
        return "eighth";
    case 9:
        return "ninth";
    case 10:
        return "tenth";
    case 11:
        return "eleventh";
    case 12:
        return "twelfth";
    case 13:
        return "thirteenth";
    case 14:
        return "fourteenth";
    case 15:
        return "fifteenth";
    case 16:
        return "sixteenth";
    case 17:
        return "seventeenth";
    case 18:
        return "eighteenth";
    case 19:
        return "nineteenth";
    case 20:
        return "twentieth";
    case 21:
        return "twenty-first";
    case 22:
        return "twenty-second";
    case 23:
        return "twenty-third";
    case 24:
        return "twenty-fourth";
    case 25:
        return "twenty-fifth";
    case 26:
        return "twenty-sixth";
    case 27:
        return "twenty-seventh";
    case 28:
        return "twenty-eighth";
    case 29:
        return "twenty-ninth";
    case 30:
        return "thirtieth";
    case 31:
        return "thirty-first";
    default:
        return ""; // Handle invalid day if necessary
    }
}

std::string formatTimestampToLLMReadable(std::tm *time)
{
    if (!time)
    {
        throw std::runtime_error("Received nullptr instead of std::tm");
    }

    // Get ordinal day (e.g., "first", "thirtieth")
    std::string dayOrdinal = formatTimestampGetOrdinalDay(time->tm_mday);

    char yearMonth[100];
    std::strftime(yearMonth, sizeof(yearMonth), "%Y %B", time);

    char timePart[10];
    std::strftime(timePart, sizeof(timePart), "%H:%M", time);

    std::stringstream ss;
    ss << yearMonth << " " << dayOrdinal << " " << timePart;

    return ss.str();
}
} // namespace RP::Utils