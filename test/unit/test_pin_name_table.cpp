#include "pin_name_table.h"
#include <gtest/gtest.h>

// EPIC-029 / TASK-380. Unit tests for the PinNameTable lookup
// container. Covered behaviours: insertion, lookup hit / miss,
// capacity guard, name-length guard, null/empty input handling,
// duplicate tolerance (first-insert wins per the TASK-378 decision).

TEST(PinNameTableTest, EmptyTableLookupReturnsUnresolved)
{
    PinNameTable t;
    EXPECT_EQ(t.lookup("button_a"), static_cast<uint8_t>(PinNameTable::kUnresolved));
    EXPECT_EQ(t.size(), 0);
}

TEST(PinNameTableTest, InsertAndLookupHit)
{
    PinNameTable t;
    ASSERT_TRUE(t.insert("button_a", 13));
    EXPECT_EQ(t.lookup("button_a"), 13);
    EXPECT_EQ(t.size(), 1);
}

TEST(PinNameTableTest, LookupMissReturnsUnresolved)
{
    PinNameTable t;
    t.insert("button_a", 13);
    EXPECT_EQ(t.lookup("button_b"), static_cast<uint8_t>(PinNameTable::kUnresolved));
}

TEST(PinNameTableTest, NamesAreCaseSensitive)
{
    PinNameTable t;
    t.insert("button_a", 13);
    EXPECT_EQ(t.lookup("BUTTON_A"), static_cast<uint8_t>(PinNameTable::kUnresolved));
}

TEST(PinNameTableTest, NullAndEmptyInputRejected)
{
    PinNameTable t;
    EXPECT_FALSE(t.insert(nullptr, 5));
    EXPECT_FALSE(t.insert("", 5));
    EXPECT_EQ(t.size(), 0);
    EXPECT_EQ(t.lookup(nullptr), static_cast<uint8_t>(PinNameTable::kUnresolved));
}

TEST(PinNameTableTest, OverlongNameRejected)
{
    PinNameTable t;
    // 32 chars + NUL would not fit (kMaxNameLen = 32 storage incl. NUL).
    std::string tooLong(64, 'x');
    EXPECT_FALSE(t.insert(tooLong.c_str(), 5));
    EXPECT_EQ(t.size(), 0);
}

TEST(PinNameTableTest, FullTableRejectsFurtherInsertions)
{
    PinNameTable t;
    // C++14 ODR rules: a static constexpr member needs an out-of-class
    // definition when ODR-used. The local int silences that without
    // changing the header.
    constexpr int kCap = PinNameTable::kMaxEntries;
    for (int i = 0; i < kCap; ++i)
    {
        std::string name = "role_" + std::to_string(i);
        ASSERT_TRUE(t.insert(name.c_str(), static_cast<uint8_t>(i)));
    }
    EXPECT_FALSE(t.insert("one_too_many", 99));
    EXPECT_EQ(static_cast<int>(t.size()), kCap);
}

TEST(PinNameTableTest, ClearResetsTable)
{
    PinNameTable t;
    t.insert("button_a", 13);
    t.insert("button_b", 12);
    t.clear();
    EXPECT_EQ(t.size(), 0);
    EXPECT_EQ(t.lookup("button_a"), static_cast<uint8_t>(PinNameTable::kUnresolved));
}

TEST(PinNameTableTest, DuplicateNameReturnsFirstInsertedPin)
{
    // TASK-378 decision: duplicate role mappings are a warning, not
    // an error. The table tolerates duplicates and returns the
    // insertion-order-first match, which matches the Dart pinOf
    // contract (lowest-numbered pin when callers insert ascending).
    PinNameTable t;
    t.insert("button_a", 13);
    t.insert("button_a", 27);
    EXPECT_EQ(t.lookup("button_a"), 13);
}

TEST(PinNameTableTest, kUnresolvedIsDistinctFromAnyValidPin)
{
    // Schema bounds pins to 0..39, so 0xFF (255) cannot collide with a
    // legitimate value. Sanity check in case the schema bound ever
    // changes.
    constexpr int kSentinel = static_cast<uint8_t>(PinNameTable::kUnresolved);
    EXPECT_EQ(kSentinel, 0xFF);
    EXPECT_GT(kSentinel, 39);
}
