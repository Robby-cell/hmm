#include <gtest/gtest.h>

#include <hmm/flat-hash-map.hpp>

// Std
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

using hmm::flat_hash_map;

// =========================================================================
// 1. Construction and Assignment
// =========================================================================

TEST(FlatHashMapTest, DefaultConstruction) {
    flat_hash_map<int, int> map;
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
}

TEST(FlatHashMapTest, InitializerListConstruction) {
    flat_hash_map<std::string, int> map{{"One", 1}, {"Two", 2}, {"Three", 3}};

    EXPECT_EQ(map.size(), 3);
    EXPECT_EQ(map.at("One"), 1);
    EXPECT_EQ(map.at("Two"), 2);
    EXPECT_EQ(map.at("Three"), 3);
}

TEST(FlatHashMapTest, CopyConstruction) {
    flat_hash_map<int, int> original;
    original.insert({1, 100});
    original.insert({2, 200});

    flat_hash_map<int, int> copy = original;

    EXPECT_EQ(copy.size(), 2);
    EXPECT_EQ(copy.at(1), 100);

    // Modify copy, ensure original is untouched
    copy[1] = 999;
    EXPECT_EQ(original.at(1), 100);
    EXPECT_EQ(copy.at(1), 999);
}

TEST(FlatHashMapTest, MoveConstruction) {
    flat_hash_map<std::string, std::string> source;
    source.insert({"key", "value"});

    flat_hash_map<std::string, std::string> dest = std::move(source);

    EXPECT_EQ(dest.size(), 1);
    EXPECT_EQ(dest.at("key"), "value");

    // Source should be empty (or valid but unspecified state)
    EXPECT_TRUE(source.empty());
}

// =========================================================================
// 2. Element Access and Modification
// =========================================================================

TEST(FlatHashMapTest, SubscriptOperator) {
    flat_hash_map<int, std::string> map;

    // Insertion via operator[]
    map[1] = "One";
    EXPECT_EQ(map.size(), 1);
    EXPECT_EQ(map.at(1), "One");

    // Modification via operator[]
    map[1] = "Uno";
    EXPECT_EQ(map.at(1), "Uno");
    EXPECT_EQ(map.size(), 1);
}

TEST(FlatHashMapTest, AtOperatorThrow) {
    flat_hash_map<int, int> map;
    map.insert({1, 10});

    EXPECT_EQ(map.at(1), 10);

    // Cast to void to suppress [[nodiscard]] warning in tests
    EXPECT_THROW((void)map.at(999), std::out_of_range);
}

// =========================================================================
// 3. Erasure and Clearing
// =========================================================================

TEST(FlatHashMapTest, EraseByKey) {
    flat_hash_map<int, int> map;
    for (int i = 0; i < 10; ++i) {
        map[i] = i;
    }

    EXPECT_EQ(map.erase(5), 1);   // Returns count of removed elements
    EXPECT_EQ(map.erase(99), 0);  // Returns 0 if not found

    // Replaced map.count(5) with find check since count() is missing
    EXPECT_EQ(map.find(5), map.end());
    EXPECT_EQ(map.size(), 9);
}

TEST(FlatHashMapTest, Clear) {
    flat_hash_map<int, int> map;
    for (int i = 0; i < 100; ++i) {
        map[i] = i;
    }

    EXPECT_FALSE(map.empty());

    map.clear();

    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
}

// =========================================================================
// 4. Advanced: Move-Only Types
// =========================================================================

TEST(FlatHashMapTest, SupportsMoveOnlyTypes) {
    // std::unique_ptr cannot be copied, only moved.
    // This tests that the map handles internal moves correctly during resizing.
    flat_hash_map<int, std::unique_ptr<int>> map;

    for (int i = 0; i < 100; ++i) {
        map.insert({i, std::make_unique<int>(i)});
    }

    // Force a resize/rehash implies moving the unique_ptrs
    map.reserve(1000);

    for (int i = 0; i < 100; ++i) {
        auto it = map.find(i);
        ASSERT_NE(it, map.end());
        EXPECT_EQ(*it->second, i);
    }
}

// =========================================================================
// 5. Stress and Resizing
// =========================================================================

TEST(FlatHashMapTest, AutomaticResizing) {
    flat_hash_map<int, int> map;
    size_t initial_cap = map.capacity();
    const int limit = 1000;

    for (int i = 0; i < limit; ++i) {
        map.insert({i, i});
    }

    EXPECT_GT(map.capacity(), initial_cap);
    EXPECT_EQ(map.size(), limit);

    // Verify data integrity after multiple resizes
    for (int i = 0; i < limit; ++i) {
        ASSERT_EQ(map.at(i), i);
    }
}

TEST(FlatHashMapTest, Reserve) {
    flat_hash_map<int, int> map;
    map.reserve(100);
    EXPECT_GE(map.capacity(), 100);

    // Insert without triggering resize (ideally)
    for (int i = 0; i < 50; ++i) {
        map[i] = i;
    }

    EXPECT_GE(map.capacity(), 100);
}

// =========================================================================
// 6. Collision Resolution
// =========================================================================

// A hasher that forces collisions by returning the same hash for everything
namespace {
struct BadHash {
    size_t operator()(int) const {
        return 0;
    }
};
}  // namespace

TEST(FlatHashMapTest, MassiveCollisions) {
    // This forces the map to use its probing strategy (linear, robin hood,
    // etc.)
    flat_hash_map<int, int, BadHash> map;

    const int count = 50;
    for (int i = 0; i < count; ++i) {
        map.insert({i, i});
    }

    EXPECT_EQ(map.size(), count);

    for (int i = 0; i < count; ++i) {
        auto it = map.find(i);
        ASSERT_NE(it, map.end())
            << "Failed to find key " << i << " despite collisions";
        EXPECT_EQ(it->second, i);
    }

    // Test erase in a high-collision environment
    map.erase(25);
    EXPECT_EQ(map.find(25), map.end());
    EXPECT_EQ(map.at(26), 26);  // Ensure probing chain isn't broken
}

// =========================================================================
// 7. Iterators
// =========================================================================

TEST(FlatHashMapTest, IteratorTraversal) {
    flat_hash_map<int, int> map;
    const int count = 10;
    for (int i = 0; i < count; ++i) {
        map[i] = i * 10;
    }

    int items_seen = 0;
    for (auto it = map.begin(); it != map.end(); ++it) {
        items_seen++;
        // Verify value matches key logic
        EXPECT_EQ(it->second, it->first * 10);
    }
    EXPECT_EQ(items_seen, count);
}

TEST(FlatHashMapTest, ConstIterator) {
    flat_hash_map<int, int> map;
    map.insert({1, 1});

    const auto& cmap = map;
    auto it = cmap.find(1);

    ASSERT_NE(it, cmap.end());
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, 1);
}

// =========================================================================
// 8. Object Lifetime (Leak Check)
// =========================================================================

namespace {
struct LifecycleTracker {
    static inline int constructions = 0;
    static inline int destructions = 0;
    int val;

    LifecycleTracker(int v) : val(v) {
        constructions++;
    }
    LifecycleTracker(const LifecycleTracker& o) : val(o.val) {
        constructions++;
    }
    LifecycleTracker(LifecycleTracker&& o) noexcept : val(o.val) {
        constructions++;
    }
    ~LifecycleTracker() {
        destructions++;
    }

    static void reset() {
        constructions = 0;
        destructions = 0;
    }
};
}  // namespace

TEST(FlatHashMapTest, ObjectLifetimeAndLeaks) {
    LifecycleTracker::reset();

    {
        flat_hash_map<int, LifecycleTracker> map;
        map.insert(std::make_pair(1, LifecycleTracker(10)));
        map.insert(std::make_pair(2, LifecycleTracker(20)));
    }

    // Assert that every construction has a matching destruction
    EXPECT_EQ(LifecycleTracker::constructions, LifecycleTracker::destructions);
}
