#include "encoder.h"

namespace RP::Encoder
{
    std::string rle(const std::string &userActivityString)
    {
        std::string output;
        std::string prevToken;
        int repeatCount = 0;
        size_t i = 0;
        while (i < userActivityString.size())
        {
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
                // If we didn't find a closing character for this token (']'), this string is malformed
                // we're now at the end of the string, so we just append everything and break out.
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
            output += "[" + prevToken + "]";
            if (repeatCount > 0)
            {
                output += "{" + std::to_string(repeatCount + 1) + "}";
            }
        }

        return output;
    }

}