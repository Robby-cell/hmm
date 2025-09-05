#include <gtest/gtest.h>

#include <array>
#include <hmm/flat-hash-set.hpp>
#include <iterator>
#include <string>
#include <utility>

using hmm::flat_hash_set;

TEST(flat_hash_set, BasicFlatHashMapTests) {
    static constexpr int Count = 10;
    flat_hash_set<int> set;

    for (int i = 0; i < Count; ++i) {
        set.insert(i);
    }

    for (int i = 0; i < Count; ++i) {
        ASSERT_TRUE(set.contains(i));
    }
}

TEST(flat_hash_set, ConstructionFromRange) {
    std::string values[]{"Hello", "World", "Foo", "Bar"};

    flat_hash_set<std::string> set{std::begin(values), std::end(values)};

    for (const auto& value : values) {
        ASSERT_TRUE(set.contains(value));
    }
}

TEST(flat_hash_set, MovingShouldWork) {
    std::string values[]{"Hello", "World", "Foo", "Bar"};

    flat_hash_set<std::string> set{std::begin(values), std::end(values)};

    auto m = std::move(set);
    for (const auto& value : values) {
        ASSERT_TRUE(m.contains(value));
        ASSERT_FALSE(set.contains(value));
    }
}

TEST(flat_hash_set, CopyingShouldWork) {
    std::string values[]{"Hello", "World", "Foo", "Bar"};

    flat_hash_set<std::string> set{std::begin(values), std::end(values)};

    auto m = set;
    for (const auto& value : values) {
        ASSERT_TRUE(m.contains(value));
        ASSERT_TRUE(set.contains(value));
    }
}
