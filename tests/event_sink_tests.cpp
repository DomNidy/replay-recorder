#include <codecvt>
#include <filesystem>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "recorder/event_sink.h"
#include "recorder/event_source.h"
#include "utils/logging.h"

class EventSinkTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create a temporary test file path
        testFilePath = "test_event_sink.txt";

        LOG_INFO("Setting up test file path: {}", testFilePath);
        // Delete the file if it exists from previous test runs
        if (std::filesystem::exists(testFilePath))
        {
            std::filesystem::remove(testFilePath);
        }
    }

    void TearDown() override
    {
        LOG_INFO("Test replay file is located at: {}", testFilePath);
    }

    std::string testFilePath;
};

TEST_F(EventSinkTest, WritesToFileAndFlushes)
{
    // Create EventSink with a shared_ptr since it uses enable_shared_from_this
    auto eventSink = std::make_shared<EventSink>(testFilePath);

    // Verify file exists as it should have been created by the EventSink constructor
    ASSERT_TRUE(std::filesystem::exists(testFilePath));

    // Write some data that's smaller than MAX_RECORDING_BUFFER_SIZE
    const wchar_t *smallData = L"This is a small test string";
    *eventSink << smallData;

    // At this point, data should be in the buffer but not yet flushed to file

    // Create data larger than MAX_RECORDING_BUFFER_SIZE to force a flush
    std::string largeData(MAX_RECORDING_BUFFER_SIZE + 100, 'X');
    *eventSink << largeData.c_str();

    // Data should now be flushed to the file
    // Read the file to verify content
    std::ifstream inFile(testFilePath);
    ASSERT_TRUE(inFile.is_open());

    std::string fileContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    // Convert wchart to char
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string smallDataStr = converter.to_bytes(smallData);
    // Verify file contains our data
    EXPECT_TRUE(fileContent.find(smallDataStr) != std::string::npos);
    EXPECT_TRUE(fileContent.find(std::string(100, 'X')) != std::string::npos);
}

// Test if we correctly add and initialize sources
TEST_F(EventSinkTest, AddsSourcesCorrectly)
{

    static int testInitVal = 0;
    static int testUninitVal = 0;
    class SomeEventSource : public EventSource
    {
      private:
        virtual void initializeSource(std::weak_ptr<EventSink> inSink)
        {
            testInitVal = 1;
        }
        virtual void uninitializeSource()
        {
            testUninitVal = 1;
        }
    };

    auto someEventSource = std::make_shared<SomeEventSource>();
    auto eventSink = std::make_shared<EventSink>(testFilePath);

    // Ensure that the event sink has no sources
    size_t numSources = eventSink->getSources().size();
    ASSERT_EQ(numSources, 0);

    // Test that event sink properly calls initalizeSource on added event sources
    eventSink->addSource(someEventSource);
    ASSERT_EQ(testInitVal, 1);

    // Check if the source was added to the sources vector
    numSources = eventSink->getSources().size();
    ASSERT_EQ(numSources, 1);

    // Check if the added source had its uninitializeSource method called
    eventSink->uninitializeSink();
    ASSERT_EQ(testUninitVal, 1);

    // Check that the source was removed from the sources vector after uninitializing the event sink
    numSources = eventSink->getSources().size();
    ASSERT_EQ(numSources, 0);
}

int main(int argc, char **argv)
{

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}