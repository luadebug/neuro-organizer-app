#include "stringUtils.h"
#include <gtest/gtest.h>

TEST(UtfFoldTest, EmptyString) {
    ASSERT_EQ(utf8_fold(""), "");
}

TEST(UtfFoldTest, SimpleAsciiFold) {
    ASSERT_EQ(utf8_fold("AbC"), "abc");
}

TEST(UtfFoldTest, UnicodeFoldGermanEszett) {
    ASSERT_EQ(utf8_fold("Straße"), "strasse");
}

TEST(UtfFoldTest, TurkishDotlessI) {
    ASSERT_EQ(utf8_fold("Iıİi"), "iıi̇i");
}
