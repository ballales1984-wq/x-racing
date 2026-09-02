#include <gtest/gtest.h>
#include "debug/console.h"

#include <string>

TEST(ConsoleTest, DefaultEmpty) {
    xe::Console c;
    EXPECT_EQ(c.OutputLines().size(), 0u);
    EXPECT_EQ(c.History().size(), 0u);
    EXPECT_FALSE(c.IsOpen());
    EXPECT_TRUE(c.Commands().empty());
}

TEST(ConsoleTest, RegisterAndList) {
    xe::Console c;
    int n = 0;
    c.Register("foo", "do foo", [&n](auto&) { n++; });
    c.Execute("foo");
    EXPECT_EQ(n, 1);
}

TEST(ConsoleTest, UnknownCommandLogsError) {
    xe::Console c;
    c.Execute("nonexistent");
    ASSERT_EQ(c.OutputLines().size(), 1u);
    EXPECT_NE(c.OutputLines()[0].find("unknown"), std::string::npos);
}

TEST(ConsoleTest, HistoryRecordsCommands) {
    xe::Console c;
    c.Register("a", "", [](auto&){});
    c.Execute("a");
    c.Execute("a arg1 arg2");
    ASSERT_EQ(c.History().size(), 2u);
    EXPECT_EQ(c.History()[0], "a");
    EXPECT_EQ(c.History()[1], "a arg1 arg2");
}

TEST(ConsoleTest, PrintAppendsOutput) {
    xe::Console c;
    c.PrintLn("hello");
    c.PrintLn("world");
    ASSERT_EQ(c.OutputLines().size(), 2u);
    EXPECT_EQ(c.OutputLines()[0], "hello");
    EXPECT_EQ(c.OutputLines()[1], "world");
}

TEST(ConsoleTest, AppendCharAndBackspace) {
    xe::Console c;
    c.AppendChar('a');
    c.AppendChar('b');
    c.AppendChar('c');
    EXPECT_EQ(c.InputLine(), "abc");
    c.Backspace();
    EXPECT_EQ(c.InputLine(), "ab");
}

TEST(ConsoleTest, HistoryNavigation) {
    xe::Console c;
    c.Register("a", "", [](auto&){});
    c.Register("b", "", [](auto&){});
    c.Execute("a");
    c.Execute("b");

    EXPECT_EQ(c.InputLine(), "");
    c.HistoryPrev();
    EXPECT_EQ(c.InputLine(), "b");
    c.HistoryPrev();
    EXPECT_EQ(c.InputLine(), "a");
    c.HistoryNext();
    EXPECT_EQ(c.InputLine(), "b");
    c.HistoryNext();
    EXPECT_EQ(c.InputLine(), "");
}

TEST(ConsoleTest, SubmitInputClearsLine) {
    xe::Console c;
    int n = 0;
    c.Register("go", "", [&n](auto&){ n++; });
    c.AppendChar('g');
    c.AppendChar('o');
    c.SubmitInput();
    EXPECT_EQ(n, 1);
    EXPECT_EQ(c.InputLine(), "");
}

TEST(ConsoleTest, OpenToggle) {
    xe::Console c;
    c.SetOpen(false);
    c.Toggle();
    EXPECT_TRUE(c.IsOpen());
    c.Toggle();
    EXPECT_FALSE(c.IsOpen());
}

TEST(ConsoleTest, ClearOutput) {
    xe::Console c;
    c.PrintLn("a");
    c.PrintLn("b");
    EXPECT_EQ(c.OutputLines().size(), 2u);
    c.ClearOutput();
    EXPECT_EQ(c.OutputLines().size(), 0u);
}

TEST(ConsoleTest, EmptyInputDoesNothing) {
    xe::Console c;
    int n = 0;
    c.Register("x", "", [&n](auto&){ n++; });
    c.Execute("");
    c.Execute("   ");
    EXPECT_EQ(n, 0);
    EXPECT_EQ(c.History().size(), 0u);
}