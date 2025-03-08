#include <gtest/gtest.h>
#include "replay/recorder/event_sink.h"

class EventSinkTest : public ::testing::Test
{

protected:
    int example_test()
    {
        return 2;
    }
};

TEST_F(EventSinkTest, EventSinkTest_HelloWorld)
{
    EXPECT_EQ(example_test(), 2);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}