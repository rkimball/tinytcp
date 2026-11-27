#include <stdio.h>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "os/output.hpp"

namespace
{

// Remembers everything passed to the callback, one entry per call.
struct Recorder
{
    std::vector<std::string> texts;
    std::vector<size_t> lengths;
};

void Record(void* context, const char* text, size_t length)
{
    Recorder* recorder = static_cast<Recorder*>(context);
    recorder->texts.push_back(std::string(text, length));
    recorder->lengths.push_back(length);
}

int g_null_context_calls = 0;

void CountWithNullContext(void* context, const char*, size_t)
{
    EXPECT_EQ(nullptr, context);
    g_null_context_calls++;
}

} // namespace

TEST(OutputTest, FormatsLikePrintf)
{
    Recorder recorder;
    os::Print(Record, &recorder, "%s=%d %05.1f %x", "answer", 42, 3.14159, 255u);
    ASSERT_EQ(1u, recorder.texts.size());
    EXPECT_EQ("answer=42 003.1 ff", recorder.texts[0]);
}

TEST(OutputTest, LengthIsTheLengthOfTheText)
{
    Recorder recorder;
    os::Print(Record, &recorder, "123456789");
    ASSERT_EQ(1u, recorder.lengths.size());
    EXPECT_EQ(9u, recorder.lengths[0]);
}

TEST(OutputTest, EachCallIsOneCallback)
{
    Recorder recorder;
    os::Print(Record, &recorder, "first\n");
    os::Print(Record, &recorder, "second\n");
    os::Print(Record, &recorder, "third %d\n", 3);
    ASSERT_EQ(3u, recorder.texts.size());
    EXPECT_EQ("first\n", recorder.texts[0]);
    EXPECT_EQ("second\n", recorder.texts[1]);
    EXPECT_EQ("third 3\n", recorder.texts[2]);
}

TEST(OutputTest, ContextIsPassedThroughUnchanged)
{
    Recorder one;
    Recorder two;
    os::Print(Record, &one, "to one");
    os::Print(Record, &two, "to two");
    ASSERT_EQ(1u, one.texts.size());
    ASSERT_EQ(1u, two.texts.size());
    EXPECT_EQ("to one", one.texts[0]);
    EXPECT_EQ("to two", two.texts[0]);
}

TEST(OutputTest, NullContextIsAllowed)
{
    // A callback that does not need a context can be given nullptr.
    g_null_context_calls = 0;
    os::Print(CountWithNullContext, nullptr, "nobody needs a context for this");
    EXPECT_EQ(1, g_null_context_calls);
}

TEST(OutputTest, EmptyFormatStillCallsBackWithZeroLength)
{
    Recorder recorder;
    os::Print(Record, &recorder, "%s", "");
    ASSERT_EQ(1u, recorder.texts.size());
    EXPECT_EQ(0u, recorder.lengths[0]);
}

TEST(OutputTest, NullOutputIsIgnored)
{
    // Nothing to check except that it does not crash.
    os::Print(nullptr, nullptr, "dropped %d", 1);
    SUCCEED();
}

TEST(OutputTest, TextThatFitsIsNotTruncated)
{
    // The buffer is 256 bytes and one is used for the terminator.
    std::string longest(255, 'x');
    Recorder recorder;
    os::Print(Record, &recorder, "%s", longest.c_str());
    ASSERT_EQ(1u, recorder.texts.size());
    EXPECT_EQ(longest, recorder.texts[0]);
}

TEST(OutputTest, LongerTextIsTruncatedTo255Characters)
{
    std::string text(256, 'a');
    Recorder recorder;
    os::Print(Record, &recorder, "%s", text.c_str());
    ASSERT_EQ(1u, recorder.texts.size());
    EXPECT_EQ(255u, recorder.lengths[0]);
    EXPECT_EQ(std::string(255, 'a'), recorder.texts[0]);

    Recorder much_longer;
    std::string huge(5000, 'b');
    os::Print(Record, &much_longer, "%s", huge.c_str());
    ASSERT_EQ(1u, much_longer.texts.size());
    EXPECT_EQ(255u, much_longer.lengths[0]);
}

TEST(OutputTest, TruncationKeepsTheStartOfTheText)
{
    std::string text = "START" + std::string(400, '.') + "END";
    Recorder recorder;
    os::Print(Record, &recorder, "%s", text.c_str());
    ASSERT_EQ(1u, recorder.texts.size());
    EXPECT_EQ("START", recorder.texts[0].substr(0, 5));
    EXPECT_EQ(std::string::npos, recorder.texts[0].find("END"));
}

TEST(OutputTest, FormattingErrorProducesNoOutput)
{
    // A wide character that cannot be converted in the "C" locale makes vsnprintf fail.
    char probe[8];
    if (snprintf(probe, sizeof(probe), "%ls", L"\xe9") >= 0)
    {
        GTEST_SKIP() << "this C library converts the wide character, so there is no error to test";
    }
    Recorder recorder;
    os::Print(Record, &recorder, "%ls", L"\xe9");
    EXPECT_TRUE(recorder.texts.empty());
}
