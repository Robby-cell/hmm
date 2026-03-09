// Copyright 2025 Robert Williamson
//
// Licensed under the MIT License;
// You may not used this file except in compliance with the License.
// You may obtain a copy of the License at
//
//       https://opensource.org/license/mit
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef HMM_HMM_INTERNAL_RAW_HASH_MAP_HPP
#define HMM_HMM_INTERNAL_RAW_HASH_MAP_HPP

#include <initializer_list>
#include <stdexcept>
#include <utility>

#include "hmm/internal/macros.hpp"
#include "hmm/internal/raw-hash-set.hpp"

namespace hmm {
namespace internal {

/// @brief The core SwissTable-style flat hash map implementation.
///
/// `raw_hash_map` builds on top of `raw_hash_set` by utilizing protected
/// inheritance. It leverages the robust and highly optimized SIMD-accelerated
/// linear probing infrastructure of the set, while safely exposing key-value
/// associative semantics to the user (such as `operator[]` and `at()`).
///
/// @tparam Policy A trait struct that dictates how keys and values are
/// extracted
///                from the underlying slot. For maps, the slot is usually a
///                `std::pair`.
/// @tparam Hash The hashing algorithm/functor used to compute the 64-bit hash.
/// @tparam Eq The equality operator used to compare keys during lookup or
/// insertion.
/// @tparam Alloc The allocator used for managing the lifecycle of the elements
/// and memory block.
template <class Policy, class Hash, class Eq, class Alloc>
class raw_hash_map : protected raw_hash_set<Policy, Hash, Eq, Alloc> {
    using Base = raw_hash_set<Policy, Hash, Eq, Alloc>;

   public:
    using policy_type = typename Base::policy_type;
    using hasher_type = typename Base::hasher_type;
    using key_equal = typename Base::key_equal;
    using key_type = typename Base::key_type;
    using value_type = typename Base::value_type;

    /// @brief The mapped value type associated with the key (exclusive to
    /// map-like containers).
    using mapped_type = typename Policy::mapped_type;
    using size_type = typename Base::size_type;
    using difference_type = typename Base::difference_type;

    using slot_type = typename Base::slot_type;
    using slot_allocator = typename Base::slot_allocator;
    using slot_traits = typename Base::slot_traits;
    using pointer = typename Base::pointer;
    using allocator_type = typename Base::allocator_type;

    using const_iterator = typename Base::const_iterator;
    using iterator = typename Base::iterator;

    /// @brief Default constructs an empty hash map.
    /// @details No memory allocations occur until the first element is
    /// inserted.
    HMM_CONSTEXPR_20 raw_hash_map() = default;

    /// @brief Constructs a hash map using an initializer list of slot values.
    /// @param initial The list of key-value pairs to populate the map with.
    /// @param alloc The allocator instance to use for memory management.
    HMM_CONSTEXPR_20 raw_hash_map(
        std::initializer_list<slot_type> initial,
        const allocator_type& alloc = allocator_type())
        : Base(initial, alloc) {}

    /// @brief Constructs a hash map from a range defined by iterators.
    /// @tparam Iter The iterator type specifying the start of the range.
    /// @tparam Sentinel The sentinel type specifying the end of the range.
    /// @param begin The start of the range to insert.
    /// @param end The end of the range to insert.
    /// @param alloc The allocator instance to use.
    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 raw_hash_map(
        Iter begin, Sentinel end,
        const allocator_type& alloc = allocator_type())
        : Base(begin, end, alloc) {}

    /// @brief Constructs an empty hash map utilizing a specific allocator.
    /// @param alloc The allocator instance to bind to the map.
    HMM_CONSTEXPR_20 explicit raw_hash_map(const allocator_type& alloc)
        : Base(alloc) {}

    // Public Base Functionality Exposure
    // The following methods are hoisted from the protected base `raw_hash_set`
    // to safely expose standard container APIs to the end user.
    using Base::begin;
    using Base::capacity;
    using Base::cbegin;
    using Base::cend;
    using Base::clear;
    using Base::contains;
    using Base::emplace;
    using Base::end;
    using Base::find;
    using Base::insert;
    using Base::reserve;
    using Base::size;
    using Base::try_emplace;

    /// @brief Erases the element at the specified iterator position.
    /// @param pos The const_iterator pointing to the element to remove.
    /// @return An iterator pointing to the element immediately following the
    /// erased element.
    HMM_CONSTEXPR_20 iterator erase(const_iterator pos) {
        return Base::erase(pos);
    }

    /// @brief Erases an element matching the provided key.
    /// @param key The key of the element to delete.
    /// @return The number of elements removed (1 if found and erased, 0
    /// otherwise).
    HMM_CONSTEXPR_20 size_type erase(const key_type& key) {
        return Base::erase_element(key);
    }

    /// @brief Accesses or inserts a value associated with the given key.
    ///
    /// @details Performs a lookup for the provided key. If the key exists, it
    /// returns a reference to its mapped value. If the key does not exist, a
    /// new element is default constructed and inserted, and a reference to the
    /// newly created mapped value is returned. If inserting the element exceeds
    /// the load factor threshold, the map is rehashed.
    ///
    /// @param key The key to search for or insert.
    /// @return A mutable reference to the `mapped_type` associated with the
    /// key.
    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& operator[](
        const key_type& key) {
        return this->try_emplace(key).first->second;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& operator[](key_type&& key) {
        return this->try_emplace(std::move(key)).first->second;
    }

    /// @brief Accesses the mapped value associated with the given key, with
    /// bounds checking.
    ///
    /// @details Searches the map for the specified key. If found, returns a
    /// mutable reference to the mapped value. Unlike `operator[]`, if the key
    /// is not found, an exception is thrown.
    ///
    /// @param key The key to look up.
    /// @return A mutable reference to the `mapped_type`.
    /// @throws std::out_of_range If the requested key does not exist within the
    /// container.
    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& at(const key_type& key) {
        auto it = this->find(key);
        auto my_end = end();
        if (it == my_end) {
            ThrowOutOfRange("raw_hash_map<>::at()");
        }
        return it->second;
    }

    /// @brief Accesses the mapped value associated with the given key, with
    /// bounds checking (const context).
    ///
    /// @param key The key to look up.
    /// @return A constant reference to the `mapped_type`.
    /// @throws std::out_of_range If the requested key does not exist within the
    /// container.
    HMM_NODISCARD HMM_CONSTEXPR_20 const mapped_type& at(
        const key_type& key) const {
        auto it = this->find(key);
        auto my_end = end();
        if (it == my_end) {
            ThrowOutOfRange("raw_hash_map<>::at()const");
        }
        return it->second;
    }

    /// @brief Internal helper method to throw an out_of_range exception
    /// cleanly.
    /// @tparam Args Variadic template arguments forwarded to the exception's
    /// constructor.
    /// @param args Forwarded arguments (usually the error message string).
    template <class... Args>
    [[noreturn]] static void ThrowOutOfRange(Args&&... args) {
        throw std::out_of_range(std::forward<Args>(args)...);
    }
};

}  // namespace internal
}  // namespace hmm

#endif  // HMM_HMM_INTERNAL_RAW_HASH_MAP_HPP
