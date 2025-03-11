#include <gtest/gtest.h>
#include <string.h>
#include "encoder/encoder.h"

class RLETest : public ::testing::Test
{
  protected:
    const std::string test_encodes_alttab()
    {
        return std::string("[ALT+TAB]");
    }
};

TEST_F(RLETest, RLETest_EncodesAltTab)
{
    EXPECT_EQ(test_encodes_alttab(), "[ALT+TAB]");
}

int main(int argc, char **argv)
{
    RP::Encoder::rle("helloworld!");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}