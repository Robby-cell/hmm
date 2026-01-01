#include <gtest/gtest.h>

#include <hmm/flat-hash-set.hpp>

// Std
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using hmm::flat_hash_set;

// =========================================================================
// 1. Construction and Assignment
// =========================================================================

TEST(FlatHashSetTest, DefaultConstruction) {
    flat_hash_set<int> set;
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0);
}

TEST(FlatHashSetTest, InitializerListConstruction) {
    flat_hash_set<std::string> set{"A", "B", "C"};
    EXPECT_EQ(set.size(), 3);
    EXPECT_TRUE(set.contains("A"));
    EXPECT_TRUE(set.contains("B"));
    EXPECT_TRUE(set.contains("C"));
    EXPECT_FALSE(set.contains("D"));
}

TEST(FlatHashSetTest, RangeConstruction) {
    std::vector<int> nums = {1, 2, 3, 4, 5};
    flat_hash_set<int> set(nums.begin(), nums.end());

    EXPECT_EQ(set.size(), 5);
    for (int i : nums) {
        EXPECT_TRUE(set.contains(i));
    }
}

TEST(FlatHashSetTest, CopyConstruction) {
    flat_hash_set<int> original{1, 2, 3};
    flat_hash_set<int> copy = original;

    EXPECT_EQ(copy.size(), 3);
    EXPECT_TRUE(copy.contains(1));

    // Modify copy, ensure original is distinct
    copy.insert(4);
    EXPECT_TRUE(copy.contains(4));
    EXPECT_FALSE(original.contains(4));
}

TEST(FlatHashSetTest, MoveConstruction) {
    flat_hash_set<std::string> source{"Moved"};
    flat_hash_set<std::string> dest = std::move(source);

    EXPECT_EQ(dest.size(), 1);
    EXPECT_TRUE(dest.contains("Moved"));

    // Source should be empty
    EXPECT_TRUE(source.empty());
}

// =========================================================================
// 2. Insertion and Access
// =========================================================================

TEST(FlatHashSetTest, InsertReturnValue) {
    flat_hash_set<int> set;

    // First insert
    auto res1 = set.insert(10);
    EXPECT_TRUE(res1.second);    // Success
    EXPECT_EQ(*res1.first, 10);  // Iterator points to 10

    // Duplicate insert
    auto res2 = set.insert(10);
    EXPECT_FALSE(res2.second);   // Failed (already exists)
    EXPECT_EQ(*res2.first, 10);  // Iterator points to existing
}

TEST(FlatHashSetTest, Emplace) {
    flat_hash_set<std::pair<int, int>> set;

    // Emplace constructs in-place
    auto res = set.emplace(1, 2);
    EXPECT_TRUE(res.second);
    EXPECT_EQ(res.first->first, 1);
    EXPECT_EQ(res.first->second, 2);

    EXPECT_TRUE(set.contains({1, 2}));
}

// =========================================================================
// 3. Erasure and Clearing
// =========================================================================

TEST(FlatHashSetTest, EraseByIterator) {
    flat_hash_set<int> set{1, 2, 3, 4, 5};

    // Locate element
    auto it = set.find(3);
    ASSERT_NE(it, set.end());

    // Erase via iterator
    set.erase(it);

    EXPECT_FALSE(set.contains(3));
    EXPECT_EQ(set.size(), 4);

    // Verify finding a non-existent key works
    EXPECT_EQ(set.find(99), set.end());
}

TEST(FlatHashSetTest, Clear) {
    flat_hash_set<int> set{1, 2, 3};
    EXPECT_FALSE(set.empty());

    set.clear();

    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.size(), 0);

    // Inserting after clear should work
    set.insert(1);
    EXPECT_TRUE(set.contains(1));
}

// =========================================================================
// 4. Advanced: Move-Only Types
// =========================================================================

// Custom hasher required because the library's default hasher
// doesn't natively support std::unique_ptr
struct UniquePtrHasher {
    size_t operator()(const std::unique_ptr<int>& ptr) const {
        return std::hash<int*>{}(ptr.get());
    }
};

TEST(FlatHashSetTest, SupportsMoveOnlyTypes) {
    // Use the custom hasher
    flat_hash_set<std::unique_ptr<int>, UniquePtrHasher> set;

    for (int i = 0; i < 100; ++i) {
        set.insert(std::make_unique<int>(i));
    }

    // Trigger resize
    set.reserve(500);

    // Verify content by iterating
    int count = 0;
    for (const auto& ptr : set) {
        if (*ptr >= 0 && *ptr < 100) {
            count++;
        }
    }
    EXPECT_EQ(count, 100);
}

// =========================================================================
// 5. Custom Types and Hashers
// =========================================================================

struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct PointHasher {
    size_t operator()(const Point& p) const {
        return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
    }
};

TEST(FlatHashSetTest, CustomKeyAndHasher) {
    flat_hash_set<Point, PointHasher> set;

    set.insert({1, 2});
    set.insert({3, 4});

    EXPECT_TRUE(set.contains({1, 2}));
    EXPECT_FALSE(set.contains({1, 3}));

    // Ensure duplicates are rejected based on custom equality
    auto res = set.insert({1, 2});
    EXPECT_FALSE(res.second);
}

// =========================================================================
// 6. Stress and Resizing
// =========================================================================

TEST(FlatHashSetTest, AutomaticResizing) {
    flat_hash_set<int> set;
    size_t initial_cap = set.capacity();
    const int limit = 1000;

    for (int i = 0; i < limit; ++i) {
        set.insert(i);
    }

    EXPECT_GT(set.capacity(), initial_cap);
    EXPECT_EQ(set.size(), limit);

    for (int i = 0; i < limit; ++i) {
        EXPECT_TRUE(set.contains(i));
    }
}

// =========================================================================
// 7. Collision Resolution
// =========================================================================

// Forces all keys to bucket 0
struct BadHash {
    size_t operator()(int) const {
        return 0;
    }
};

TEST(FlatHashSetTest, MassiveCollisions) {
    flat_hash_set<int, BadHash> set;
    const int count = 50;

    for (int i = 0; i < count; ++i) {
        set.insert(i);
    }

    EXPECT_EQ(set.size(), count);

    // Find must still work despite everything hashing to 0
    for (int i = 0; i < count; ++i) {
        EXPECT_TRUE(set.contains(i));
    }

    // Removal in collision chain using iterator
    auto it = set.find(25);
    ASSERT_NE(it, set.end());
    set.erase(it);

    EXPECT_FALSE(set.contains(25));
    EXPECT_TRUE(set.contains(26));  // Chain shouldn't break
}

// =========================================================================
// 8. Iterators
// =========================================================================

TEST(FlatHashSetTest, IteratorTraversal) {
    flat_hash_set<int> set;
    for (int i = 0; i < 10; ++i) {
        set.insert(i);
    }

    int sum = 0;
    for (auto it = set.begin(); it != set.end(); ++it) {
        sum += *it;
    }
    // Sum of 0..9 is 45
    EXPECT_EQ(sum, 45);
}

// =========================================================================
// 9. Object Lifetime (Leak Check)
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

    bool operator==(const LifecycleTracker& other) const {
        return val == other.val;
    }

    ~LifecycleTracker() {
        destructions++;
    }

    static void reset() {
        constructions = 0;
        destructions = 0;
    }
};

struct LifecycleHasher {
    size_t operator()(const LifecycleTracker& l) const {
        return std::hash<int>{}(l.val);
    }
};
}  // namespace

TEST(FlatHashSetTest, ObjectLifetimeAndLeaks) {
    LifecycleTracker::reset();

    {
        flat_hash_set<LifecycleTracker, LifecycleHasher> set;
        set.emplace(10);
        set.emplace(20);
        set.emplace(10);  // Duplicate

        EXPECT_EQ(set.size(), 2);
    }
    // set goes out of scope

    // Everything constructed must be destroyed
    EXPECT_EQ(LifecycleTracker::constructions, LifecycleTracker::destructions);
}
