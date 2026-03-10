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

#ifndef HMM_HMM_FLAT_HASH_MAP_HPP
#define HMM_HMM_FLAT_HASH_MAP_HPP

#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>

#include "hmm/city-hash.hpp"
#include "hmm/internal/macros.hpp"
#include "hmm/internal/raw-hash-map.hpp"

#if HMM_HAS_CXX_17
#include <memory_resource>
#endif

namespace hmm {

/// @brief Forward declaration of the map policy traits.
template <typename K, typename V>
struct MapPolicy;

/// @brief A high-performance, SIMD-accelerated flat hash map.
///
/// `flat_hash_map` is an unordered associative container that stores key-value
/// pairs. It acts as a highly optimized, drop-in replacement for
/// `std::unordered_map`. Under the hood, it uses a SwissTable-inspired
/// open-addressing architecture with linear probing and 16-way SIMD metadata
/// parallel lookups.
///
/// Unlike node-based containers (`std::unordered_map`), `flat_hash_map` stores
/// elements in a single contiguous memory allocation, vastly improving cache
/// locality and iteration speeds.
///
/// @tparam Key The type of keys stored in the map.
/// @tparam Value The type of mapped values.
/// @tparam Hash The hashing functor. Defaults to `hmm::CityHash<Key>`.
/// @tparam KeyEqual The equality functor. Defaults to `std::equal_to<Key>`.
/// @tparam Alloc The allocator used for memory management.
template <class Key, class Value, class Hash = CityHash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Alloc =
              std::allocator<typename MapPolicy<Key, Value>::slot_type>>
class flat_hash_map : protected internal::raw_hash_map<MapPolicy<Key, Value>,
                                                       Hash, KeyEqual, Alloc> {
    using Base =
        internal::raw_hash_map<MapPolicy<Key, Value>, Hash, KeyEqual, Alloc>;

   public:
    using policy_type = typename Base::policy_type;
    using hasher_type = typename Base::hasher_type;
    using key_equal = typename Base::key_equal;
    using key_type = typename Base::key_type;
    using value_type = typename Base::value_type;
    using mapped_type = typename Base::mapped_type;
    using size_type = typename Base::size_type;
    using difference_type = typename Base::difference_type;

    using slot_type = typename Base::slot_type;
    using slot_allocator = typename Base::slot_allocator;
    using slot_traits = typename Base::slot_traits;
    using pointer = typename Base::pointer;
    using allocator_type = typename Base::allocator_type;

    using const_iterator = typename Base::const_iterator;
    using iterator = typename Base::iterator;

    /// @brief Default constructs an empty flat hash map.
    HMM_CONSTEXPR_20 flat_hash_map() = default;

    /// @brief Constructs the map with the contents of an initializer list.
    /// @param initial The std::initializer_list of key-value pairs.
    /// @param alloc The allocator instance to use.
    HMM_CONSTEXPR_20 flat_hash_map(
        std::initializer_list<slot_type> initial,
        const allocator_type& alloc = allocator_type())
        : Base(initial.begin(), initial.end(), alloc) {}

    /// @brief Constructs the map with the contents of a range.
    /// @tparam Iter Iterator type.
    /// @tparam Sentinel Sentinel type denoting the end of the range.
    /// @param begin The beginning of the range.
    /// @param end The end of the range.
    /// @param alloc The allocator instance to use.
    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 flat_hash_map(
        Iter begin, Sentinel end,
        const allocator_type& alloc = allocator_type())
        : Base(begin, end, alloc) {}

    /// @brief Constructs an empty map utilizing a specific allocator.
    /// @param alloc The allocator instance.
    HMM_CONSTEXPR_20
    explicit flat_hash_map(const allocator_type& alloc) : Base(alloc) {}

    /// @name Standard Container Interfaces
    /// These functions provide standard capacity, iteration, and modifier
    /// interfaces compliant with standard C++ container requirements.
    ///@{
    using Base::begin;
    using Base::capacity;
    using Base::cbegin;
    using Base::cend;
    using Base::clear;
    using Base::emplace;
    using Base::empty;
    using Base::end;
    using Base::erase;
    using Base::erase_element;
    using Base::insert;
    using Base::reserve;
    using Base::size;
    ///@}

    /// @brief Finds an element with the exact specified key.
    /// @param key The key to look up.
    /// @return An iterator to the found element, or `end()` if not found.
    HMM_NODISCARD HMM_CONSTEXPR_20 iterator find(const key_type& key) {
        return Base::find(key);
    }

    /// @brief Finds an element with the exact specified key (const context).
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator
    find(const key_type& key) const {
        return Base::find(key);
    }

    /// @brief Checks if an element with the exact specified key exists.
    /// @param key The key to look up.
    /// @return `true` if the key exists, `false` otherwise.
    HMM_NODISCARD HMM_CONSTEXPR_20 bool contains(const key_type& key) const {
        return Base::contains(key);
    }

    /// @brief Heterogeneous lookup for a key.
    /// @details Allows looking up elements using an equivalent but different
    /// key type (e.g., `std::string_view` for a map keyed by `std::string`)
    /// without allocating a temporary object, provided the `Hash` and
    /// `KeyEqual` types support `is_transparent`.
    /// @tparam K The type of the key being queried.
    /// @param key The key to look up.
    /// @return An iterator to the found element, or `end()` if not found.
    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 iterator find(const K& key) {
        return Base::find(key);
    }

    /// @brief Heterogeneous lookup for a key (const context).
    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator find(const K& key) const {
        return Base::find(key);
    }

    /// @brief Heterogeneous check if a key exists.
    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 bool contains(const K& key) const {
        return Base::contains(key);
    }

    /// @brief Attempts to construct an element in-place, avoiding allocation if
    /// the key exists.
    /// @details If the key already exists, no arguments are evaluated or moved
    /// from. If the key does not exist, the key and value are piecewise
    /// constructed in the map's storage.
    /// @tparam K The key type (forwarded).
    /// @tparam Args The types of the arguments to construct the mapped value.
    /// @param key The key to insert.
    /// @param args The arguments to forward to the mapped value's constructor.
    /// @return A pair consisting of an iterator to the inserted (or existing)
    /// element, and a bool indicating whether insertion actually occurred.
    template <class K, class... Args>
    HMM_CONSTEXPR_20 std::pair<iterator, bool> try_emplace(K&& key,
                                                           Args&&... args) {
        return Base::try_emplace(std::forward<K>(key),
                                 std::forward<Args>(args)...);
    }

    /// @brief Accesses the mapped value associated with the key, inserting a
    /// default-constructed value if the key does not already exist.
    /// @param key The key to access.
    /// @return A reference to the associated mapped value.
    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& operator[](
        const key_type& key) {
        return Base::operator[](key);
    }

    /// @brief Accesses the mapped value using a movable key, inserting a
    /// default-constructed value via move semantics if the key does not already
    /// exist.
    /// @param key The key to access and potentially move from.
    /// @return A reference to the associated mapped value.
    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& operator[](key_type&& key) {
        return Base::operator[](std::move(key));
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& at(const key_type& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("flat_hash_map::at");
        }
        return it->second;
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 const mapped_type& at(
        const key_type& key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("flat_hash_map::at");
        }
        return it->second;
    }
};

template <typename K, typename V>
struct MapPolicy {
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K, V>;
    using slot_type = std::pair<K, V>;

    HMM_NODISCARD static constexpr const key_type& key(
        const slot_type& pair) noexcept {
        return pair.first;
    }

    HMM_NODISCARD static constexpr const key_type& key(
        const value_type& pair) noexcept {
        return pair.first;
    }

    HMM_NODISCARD static constexpr mapped_type& value(
        slot_type& pair) noexcept {
        return pair.second;
    }

    HMM_NODISCARD static constexpr const mapped_type& value(
        const slot_type& pair) noexcept {
        return pair.second;
    }

    HMM_NODISCARD static value_type& value_from_slot(slot_type& slot) noexcept {
        return reinterpret_cast<value_type&>(slot);
    }

    HMM_NODISCARD static const value_type& value_from_slot(
        const slot_type& slot) noexcept {
        return reinterpret_cast<const value_type&>(slot);
    }

    template <class Alloc, class... Args>
    static HMM_CONSTEXPR_20 void construct(Alloc& alloc, slot_type* ptr,
                                           Args&&... args) {
        std::allocator_traits<Alloc>::construct(alloc, ptr,
                                                std::forward<Args>(args)...);
    }

    template <class Alloc>
    static HMM_CONSTEXPR_20 void destroy(Alloc& alloc, slot_type* ptr) {
        std::allocator_traits<Alloc>::destroy(alloc, ptr);
    }
};

#if HMM_HAS_CXX_17
namespace pmr {
template <class Key, class Value, class Hash = CityHash<Key>,
          class Eq = std::equal_to<Key>>
using flat_hash_map = ::hmm::flat_hash_map<
    Key, Value, Hash, Eq,
    std::pmr::polymorphic_allocator<typename MapPolicy<Key, Value>::slot_type>>;
}
#endif

}  // namespace hmm

#endif  // HMM_HMM_FLAT_HASH_MAP_HPP
