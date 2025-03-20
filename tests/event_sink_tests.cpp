#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "recorder/event_sink.h"

class EventSinkTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create a temporary test file path
        testFilePath = "test_event_sink.txt";

        // Delete the file if it exists from previous test runs
        if (std::filesystem::exists(testFilePath))
        {
            std::filesystem::remove(testFilePath);
        }
    }

    void TearDown() override
    {
        // Clean up the test file
        if (std::filesystem::exists(testFilePath))
        {
            std::filesystem::remove(testFilePath);
        }
    }

    std::string testFilePath;
};

TEST_F(EventSinkTest, WritesToFileAndFlushes)
{
    // Create EventSink with a shared_ptr since it uses enable_shared_from_this
    auto eventSink = std::make_shared<EventSink>(testFilePath);

    // Write some data that's smaller than MAX_RECORDING_BUFFER_SIZE
    const char *smallData = "This is a small test string";
    *eventSink << smallData;

    // At this point, data should be in the buffer but not yet flushed to file

    // Verify file exists but might be empty
    ASSERT_TRUE(std::filesystem::exists(testFilePath));

    // Create data larger than MAX_RECORDING_BUFFER_SIZE to force a flush
    std::string largeData(MAX_RECORDING_BUFFER_SIZE + 100, 'X');
    *eventSink << largeData.c_str();

    // Data should now be flushed to the file
    // Read the file to verify content
    std::ifstream inFile(testFilePath);
    ASSERT_TRUE(inFile.is_open());

    std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    // Verify file contains our data
    EXPECT_TRUE(fileContent.find(smallData) != std::string::npos);
    EXPECT_TRUE(fileContent.find(std::string(100, 'X')) != std::string::npos);
}

int main(int argc, char **argv)
{

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}