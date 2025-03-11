#include <gtest/gtest.h>
#include <string.h>
#include "encoder/encoder.h"

class RLETest : public ::testing::Test
{
};

TEST(RLETest, RepeatedCharactersCompression)
{
    EXPECT_EQ(RP::Encoder::rle("AAAA"), "A{4}");
}

TEST(RLETest, RepeatedCharactersBelowThreshold)
{
    EXPECT_EQ(RP::Encoder::rle("AAA"), "AAA");
}

TEST(RLETest, NoCompressionForDistinctCharacters)
{
    EXPECT_EQ(RP::Encoder::rle("ABC"), "ABC");
}

TEST(RLETest, MixedRepeatedCharactersBelowThreshold)
{
    // "AAABBBCCC" – each group is below threshold so they remain literal.
    EXPECT_EQ(RP::Encoder::rle("AAABBBCCC"), "AAABBBCCC");
}

TEST(RLETest, SingleSpecialToken)
{
    EXPECT_EQ(RP::Encoder::rle("[LSHIFT]"), "[LSHIFT]");
}

TEST(RLETest, RepeatedSpecialTokensTwice)
{
    EXPECT_EQ(RP::Encoder::rle("[SPACE][SPACE]"), "[SPACEx2]");
}

TEST(RLETest, RepeatedSpecialTokensThrice)
{
    EXPECT_EQ(RP::Encoder::rle("[SPACE][SPACE][SPACE]"), "[SPACEx3]");
}

TEST(RLETest, MixedSpecialAndCharacters)
{
    EXPECT_EQ(RP::Encoder::rle("B[LSHIFT]AA"), "B[LSHIFT]AA");
}

// Test: Malformed special token (missing closing ']') should be left as is.
TEST(RLETest, MalformedSpecialToken)
{
    EXPECT_EQ(RP::Encoder::rle("B[LSHIFT AAAA"), "B[LSHIFT AAAA");
}

// Test: Escaped special tokens should not be treated as special tokens.
TEST(RLETest, EscapedSpecialToken)
{
    // In "A\\[BBB", the "[" is escaped (preceded by a backslash) so the whole string is treated as literal.
    std::string input = "A\\[BBB";
    EXPECT_EQ(RP::Encoder::rle(input), input);
}

// Test: Complex mixed case including compressed special tokens and character repeated compression.
TEST(RLETest, ComplexMixedInput)
{
    // First, _rle_special_tokens processes "[A][A]" to "[Ax2]". Then, _rle_character_tokens leaves "BBB" as "BBB"
    // (since 3 < 4) and compresses "CCCC" to "C{4}". The combined expected result is:
    EXPECT_EQ(RP::Encoder::rle("[SPACE][SPACE][LSHIFT]BBBCCCCC"), "[SPACEx2][LSHIFT]BBBC{5}");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}