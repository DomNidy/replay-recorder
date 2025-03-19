#include <gtest/gtest.h>
#include "utils/timestamp_utils.h"

class UtilsTest : public ::testing::Test
{
};

int main(int argc, char **argv)
{
    RP::Utils::formatTimestampGetOrdinalDay(1);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}