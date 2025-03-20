#include <spdlog/spdlog.h>
#include "encoder.h"

inline std::string _rle_special_tokens(const std::string &userActivityString);
inline std::string _rle_character_tokens(const std::string &userActivityString);
namespace RP::Encoder
{
std::string rle(const std::string &userActivityString)
{
    std::string encodedString = _rle_special_tokens(userActivityString);
    encodedString = _rle_character_tokens(encodedString);
    return encodedString;
}

} // namespace RP::Encoder

inline std::string _rle_special_tokens(const std::string &userActivityString)
{
    std::string output;
    std::string prevToken;
    int repeatCount = 0;
    size_t i = 0;
    while (i < userActivityString.size())
    {
        // Check for special tokens: If start token ('[') is found and it's not escaped
        if (userActivityString[i] == '[' && (i == 0 || userActivityString[i - 1] != '\\'))
        {
            // Parse the token
            size_t j = i + 1;
            bool found = false;
            while (j < userActivityString.size())
            {
                if (userActivityString[j] == ']' && (j == 0 || userActivityString[j - 1] != '\\'))
                {
                    found = true;
                    break;
                }
                j++;
            }
            // If we didn't find a closing character for this token (']'), this string
            // is malformed we're now at the end of the string, so we just append
            // everything and break out.
            if (!found)
            {
                if (!prevToken.empty())
                {
                    output += "[" + prevToken + "x" + std::to_string(repeatCount + 1) + "]";
                }
                output += userActivityString.substr(i);
                prevToken.clear();
                break;
            }
            std::string tokenContent = userActivityString.substr(i + 1, j - (i + 1));

            spdlog::debug("tokenContent: {}", tokenContent);
            if (tokenContent == prevToken)
            {
                repeatCount++;
            }
            else
            {
                if (!prevToken.empty())
                {
                    output += "[" + prevToken;
                    if (repeatCount > 0)
                    {
                        output += "x" + std::to_string(repeatCount + 1);
                    }
                    output += "]";
                }
                prevToken = tokenContent;
                repeatCount = 0;
            }
            i = j + 1;
        }
        else
        {
            // Flush any pending token
            if (!prevToken.empty())
            {
                output += "[" + prevToken;
                if (repeatCount > 0)
                {
                    output += "x" + std::to_string(repeatCount + 1);
                }
                output += "]";
                prevToken.clear();
                repeatCount = 0;
            }
            // Add current character to output
            output += userActivityString[i];
            i++;
        }
    }

    // Flush any remaining token after processing all characters
    if (!prevToken.empty())
    {
        output += "[" + prevToken;
        if (repeatCount > 0)
        {
            output += "x" + std::to_string(repeatCount + 1);
        }
        output += "]";
    }

    return output;
}

inline std::string _rle_character_tokens(const std::string &userActivityString)
{
    std::string output;
    size_t i = 0;

    while (i < userActivityString.length())
    {
        char tmp = userActivityString[i];
        size_t k = 0;

        // Ignore special tokens
        if (userActivityString[i] == '[' && (i == 0 || userActivityString[i - 1] != '\\'))
        {
            // Find closing special token (']')
            while (i + k < userActivityString.length() &&
                   !(userActivityString[i + k] == ']' && userActivityString[i + k - 1] != '\\'))
            {
                k++;
            }

            // copy the contents of special token into output str
            output += userActivityString.substr(i, k + 1);
            // update i to first char after closing token
            i = i + k + 1;
            continue;
        }
        else
            while (i + k < userActivityString.length() && userActivityString[i + k] == tmp)
            {
                k++;
            }

        // Only encode when char occurs at least 4 times, since the min length of enc is 4
        output += k >= 4 ? std::string(1, tmp) + "{" + std::to_string(k) + "}" : std::string(k, tmp);
        i = i + k;
    }

    return output;
}