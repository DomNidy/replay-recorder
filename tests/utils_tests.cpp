#include <gtest/gtest.h>
#include "utils/utils.h"

class UtilsTest : public ::testing::Test
{
};

int main(int argc, char **argv)
{
    RP::Utils::helloWorld();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}