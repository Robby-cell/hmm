#include <gtest/gtest.h>

#include <hmm/flat-hash-map.hpp>

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
