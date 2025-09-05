#include <gtest/gtest.h>

#include <hmm/flat-hash-map.hpp>

// Std
#include <string>
#include <utility>

using hmm::flat_hash_map;

TEST(flat_hash_map, BasicFlatHashMapTests) {
    static constexpr int Count = 10;
    flat_hash_map<int, int> map;

    for (int i = 0; i < Count; ++i) {
        map.insert({i, i + 1});
    }

    for (int i = 0; i < Count; ++i) {
        ASSERT_EQ(map.at(i), i + 1);
    }
}

TEST(flat_hash_map, InitializerListConstruction) {
    const std::pair<const std::string, std::string> pair1{"Hello", "World"};
    const std::pair<const std::string, std::string> pair2{"Foo", "Bar"};

    flat_hash_map<std::string, std::string> map{
        pair1,
        pair2,
    };

    ASSERT_EQ(map.at(pair1.first), pair1.second);
    ASSERT_EQ(map.at(pair2.first), pair2.second);
}

TEST(flat_hash_map, ProveItGrowsIfNeeded) {
    static constexpr int Count = 32;
    flat_hash_map<int, int> map;
    for (int i = 0; i < Count; ++i) {
        map.insert({i, i});
    }
    for (int i = 0; i < Count; i += 3) {
        ASSERT_EQ(map.at(i), i);
    }
    ASSERT_GT(map.capacity(), Count);
}
