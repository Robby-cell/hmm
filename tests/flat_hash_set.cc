#include <gtest/gtest.h>

#include <algorithm>
#include <hmm/flat-hash-set.hpp>

// Std
#include <array>
#include <cstddef>
#include <functional>
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
    const std::string values[]{"Hello", "World", "Foo", "Bar"};

    const flat_hash_set<std::string> set{std::begin(values), std::end(values)};

    for (const auto& value : values) {
        ASSERT_TRUE(set.contains(value));
    }
}

TEST(flat_hash_set, MovingShouldWork) {
    const std::string values[]{"Hello", "World", "Foo", "Bar"};

    flat_hash_set<std::string> moved{std::begin(values), std::end(values)};

    const auto set = std::move(moved);
    for (const auto& value : values) {
        ASSERT_TRUE(set.contains(value));
        ASSERT_FALSE(moved.contains(value));
    }
}

TEST(flat_hash_set, CopyingShouldWork) {
    const std::string values[]{"Hello", "World", "Foo", "Bar"};

    flat_hash_set<std::string> copied{std::begin(values), std::end(values)};

    const auto set = copied;
    for (const auto& value : values) {
        ASSERT_TRUE(set.contains(value));
        ASSERT_TRUE(copied.contains(value));
    }
}

TEST(flat_hash_set, CustomKeysShouldWork) {
    struct CustomKey {
        int x;
    };
    struct CustomKeyHasher {
        std::size_t operator()(const CustomKey& k) const noexcept {
            return std::hash<int>{}(k.x);
        }
    };
    struct CustomKeyEq {
        bool operator()(const CustomKey& lhs,
                        const CustomKey& rhs) const noexcept {
            return lhs.x == rhs.x;
        }
    };

    flat_hash_set<CustomKey, CustomKeyHasher, CustomKeyEq> set;

    const auto successful = set.emplace(42);
    ASSERT_TRUE(successful.second);

    const auto result = set.emplace(42);
    ASSERT_FALSE(result.second);
}

namespace test_CustomerHasherOverload {
struct CustomKey {
    int x;
    template <typename H>
    friend H HashValue(H h, const CustomKey& self) {
        return H::combine(std::move(h), self.x);
    }
    friend bool operator==(const CustomKey& self,
                           const CustomKey& that) noexcept {
        return self.x == that.x;
    }
};
}  // namespace test_CustomerHasherOverload
TEST(flat_hash_set, CustomHasherOverload) {
    using namespace test_CustomerHasherOverload;  // NOLINT

    flat_hash_set<CustomKey> set;

    const auto successful = set.emplace(42);
    ASSERT_TRUE(successful.second);

    const auto result = set.emplace(42);
    ASSERT_FALSE(result.second);

    for (int i = 0; i < 10; ++i) {
        set.emplace(i);
    }
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(set.contains({i}));
    }
    ASSERT_TRUE(set.contains(CustomKey{42}));
}
