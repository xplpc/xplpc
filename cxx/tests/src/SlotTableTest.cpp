#include "xplpc/core/SlotTable.hpp"
#include "xplpc/core/Handle.hpp"
#include "gtest/gtest.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

using namespace xplpc::core;

TEST(SlotTableTest, HandleIsNeverZero)
{
    // Zero is what a caller reads as an answer that already arrived, so no call can ever be handed it.

    SlotTable<int> table;
    std::vector<Handle> handles;

    for (int i = 0; i < 64; i++)
    {
        handles.push_back(table.insert(i));
    }

    for (const auto handle : handles)
    {
        EXPECT_NE(0u, handle);
        table.take(handle);
    }

    for (int i = 0; i < 64; i++)
    {
        EXPECT_NE(0u, table.insert(i));
    }
}

TEST(SlotTableTest, TakenHandleIsRefused)
{
    SlotTable<int> table;

    const auto handle = table.insert(7);

    ASSERT_NE(nullptr, table.find(handle));
    EXPECT_EQ(1u, table.count());

    EXPECT_EQ(std::optional<int>{7}, table.take(handle));
    EXPECT_EQ(0u, table.count());

    EXPECT_EQ(nullptr, table.find(handle));
    EXPECT_EQ(std::nullopt, table.take(handle));
}

TEST(SlotTableTest, ReusedSlotRefusesTheOldHandle)
{
    // A late answer carrying the handle of a call that is gone must not resolve the newer call that took its slot.

    SlotTable<int> table;

    const auto first = table.insert(1);
    table.take(first);

    const auto second = table.insert(2);

    ASSERT_EQ(HandleParts::slot(first), HandleParts::slot(second));
    EXPECT_NE(first, second);

    EXPECT_EQ(nullptr, table.find(first));
    EXPECT_EQ(std::nullopt, table.take(first));

    ASSERT_NE(nullptr, table.find(second));
    EXPECT_EQ(2, *table.find(second));
    EXPECT_EQ(1u, table.count());
}

TEST(SlotTableTest, HandleOfASlotNeverAllocatedIsRefused)
{
    SlotTable<int> table;
    table.insert(1);

    EXPECT_EQ(nullptr, table.find(0));
    EXPECT_EQ(nullptr, table.find(HandleParts::compose(5, HandleParts::firstGeneration)));
    EXPECT_EQ(std::nullopt, table.take(HandleParts::compose(5, HandleParts::firstGeneration)));
}

TEST(SlotTableTest, GenerationWrapsPastZeroToOne)
{
    // Zero is skipped when the generation wraps, since slot zero with generation zero would compose the handle zero.

    EXPECT_EQ(HandleParts::firstGeneration, HandleParts::next(std::numeric_limits<std::uint32_t>::max()));
    EXPECT_EQ(2u, HandleParts::next(1));
    EXPECT_NE(0u, HandleParts::compose(0, HandleParts::next(std::numeric_limits<std::uint32_t>::max())));
}

TEST(SlotTableTest, HandleCarriesItsSlotAndGeneration)
{
    const auto handle = HandleParts::compose(0xdeadbeef, 0xcafebabe);

    EXPECT_EQ(0xdeadbeefu, HandleParts::slot(handle));
    EXPECT_EQ(0xcafebabeu, HandleParts::generation(handle));
}

TEST(SlotTableTest, SelectAnswersTheHandlesOfTheMatchingValues)
{
    SlotTable<int> table;

    const auto even = table.insert(2);
    const auto odd = table.insert(3);

    // clang-format off
    const auto selected = table.select([](int value) {
        return value % 2 == 0;
    });
    // clang-format on

    ASSERT_EQ(1u, selected.size());
    EXPECT_EQ(even, selected.front());
    EXPECT_NE(odd, selected.front());
}

TEST(SlotTableTest, EveryHandleIsExactAsAJavascriptNumber)
{
    // The largest slot and generation a handle can carry still fits the integers a double holds exactly.

    const auto largest = HandleParts::compose(HandleParts::slotLimit - 1, UINT32_MAX);

    EXPECT_LE(largest, (std::uint64_t{1} << 53) - 1);
}

TEST(SlotTableTest, InsertAnswersZeroOnceEverySlotIsTaken)
{
    SlotTable<int> table;

    for (std::uint32_t slot = 0; slot < HandleParts::slotLimit; slot++)
    {
        ASSERT_NE(0u, table.insert(1));
    }

    EXPECT_EQ(0u, table.insert(1));
}
