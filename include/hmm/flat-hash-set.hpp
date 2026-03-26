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

#ifndef HMM_HMM_FLAT_HASH_SET_HPP
#define HMM_HMM_FLAT_HASH_SET_HPP

#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>

#include "hmm/city-hash.hpp"
#include "hmm/internal/macros.hpp"
#include "hmm/internal/raw-hash-set.hpp"

#if HMM_HAS_CXX_17
#include <memory_resource>
#endif

namespace hmm {

/// @brief Forward declaration of the set policy traits.
template <typename T> struct SetPolicy;

/// @brief A high-performance, SIMD-accelerated flat hash set.
///
/// `flat_hash_set` is an unordered associative container that stores unique
/// elements. It acts as a highly optimized, drop-in replacement for
/// `std::unordered_set`. Under the hood, it uses a SwissTable-inspired
/// open-addressing architecture with linear probing and SIMD metadata parallel
/// lookups.
///
/// Unlike node-based containers (`std::unordered_set`), `flat_hash_set` stores
/// elements in a single contiguous memory allocation, vastly improving cache
/// locality and iteration speeds.
///
/// @tparam Contained The type of elements stored in the set.
/// @tparam TArgs Variadic template arguments specifying optionally:
///               1. Hash functor (Defaults to `hmm::CityHash<Contained>`)
///               2. Equality functor (Defaults to `std::equal_to<Contained>`)
///               3. Allocator (Defaults to `std::allocator<Contained>`)
template <class Contained, class... TArgs>
class flat_hash_set
    : protected internal::raw_hash_set<SetPolicy<Contained>, TArgs...> {
    using Base = internal::raw_hash_set<SetPolicy<Contained>, TArgs...>;

  public:
    using policy_type = typename Base::policy_type;
    using hasher_type = typename Base::hasher_type;
    using key_equal = typename Base::key_equal;
    using key_type = typename Base::key_type;
    using value_type = typename Base::value_type;
    using size_type = typename Base::size_type;
    using difference_type = typename Base::difference_type;

    using slot_type = typename Base::slot_type;
    using slot_allocator = typename Base::slot_allocator;
    using slot_traits = typename Base::slot_traits;
    using pointer = typename Base::pointer;
    using allocator_type = typename Base::allocator_type;

    using iterator = typename Base::const_iterator;
    using const_iterator = typename Base::const_iterator;

  public:
    /// @brief Default constructs an empty flat hash set.
    HMM_CONSTEXPR_20 flat_hash_set() = default;

    /// @brief Constructs the set with the contents of an initializer list.
    /// @param initial The std::initializer_list of elements.
    /// @param alloc The allocator instance to use.
    HMM_CONSTEXPR_20
    flat_hash_set(std::initializer_list<slot_type> initial,
                  const allocator_type& alloc = allocator_type())
        : Base(initial.begin(), initial.end(), alloc) {}

    /// @brief Constructs the set with the contents of a range.
    /// @tparam Iter Iterator type.
    /// @tparam Sentinel Sentinel type denoting the end of the range.
    /// @param begin The beginning of the range.
    /// @param end The end of the range.
    /// @param alloc The allocator instance to use.
    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20
    flat_hash_set(Iter begin, Sentinel end,
                  const allocator_type& alloc = allocator_type())
        : Base(begin, end, alloc) {}

    /// @brief Constructs an empty set utilizing a specific allocator.
    /// @param alloc The allocator instance.
    HMM_CONSTEXPR_20 explicit flat_hash_set(const allocator_type& alloc)
        : Base(alloc) {}

    /// @name Standard Container Interfaces
    /// These functions provide standard capacity, iteration, and modifier
    /// interfaces compliant with standard C++ container requirements.
    ///@{
    using Base::begin;
    using Base::capacity;
    using Base::cbegin;
    using Base::cend;
    using Base::clear;
    using Base::empty;
    using Base::end;
    using Base::erase;
    using Base::erase_element;
    using Base::reserve;
    using Base::size;
    ///@}

    /// @brief Inserts a new element into the set if it does not already exist.
    /// @param value The value to insert.
    /// @return A pair consisting of an iterator to the inserted (or existing)
    /// element,
    ///         and a bool indicating whether insertion actually occurred.
    std::pair<iterator, bool> insert(const value_type& value) {
        return Base::insert(value);
    }

    /// @brief Inserts a new element into the set via move semantics.
    /// @param value The value to move-insert.
    /// @return A pair consisting of an iterator to the inserted (or existing)
    /// element,
    ///         and a bool indicating whether insertion actually occurred.
    std::pair<iterator, bool> insert(value_type&& value) {
        return Base::insert(std::move(value));
    }

    /// @brief Inserts a range of elements into the set.
    /// @tparam InputIt Iterator type.
    /// @param first The start of the range.
    /// @param last The end of the range.
    template <class InputIt> void insert(InputIt first, InputIt last) {
        return Base::insert(first, last);
    }

    /// @brief Inserts a list of elements into the set.
    /// @param ilist The initializer list of elements to insert.
    void insert(std::initializer_list<value_type> ilist) {
        return insert(ilist.begin(), ilist.end());
    }

    /// @brief Attempts to construct an element in-place, avoiding allocation if
    /// the key already exists.
    /// @tparam Args The types of the arguments to construct the element.
    /// @param args The arguments to forward to the element's constructor.
    /// @return A pair consisting of an iterator to the inserted (or existing)
    ///         element, and a bool indicating whether insertion actually
    ///         occurred.
    template <class... Args> std::pair<iterator, bool> emplace(Args&&... args) {
        return Base::emplace(std::forward<Args>(args)...);
    }

    /// @brief Finds an element with the exact specified key.
    /// @param key The key to look up.
    /// @return An iterator to the found element, or `end()` if not found.
    HMM_NODISCARD HMM_CONSTEXPR_20 iterator find(const key_type& key) const {
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
    /// key type (e.g., `std::string_view` for a set containing `std::string`)
    /// without allocating a temporary object, provided the `Hash` and
    /// `KeyEqual` types support `is_transparent`.
    /// @tparam K The type of the key being queried.
    /// @param key The key to look up.
    /// @return An iterator to the found element, or `end()` if not found.
    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 iterator find(const K& key) const {
        return Base::find(key);
    }

    /// @brief Heterogeneous check if a key exists.
    /// @tparam K The type of the key being queried.
    /// @param key The key to look up.
    /// @return `true` if the key exists, `false` otherwise.
    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 bool contains(const K& key) const {
        return Base::contains(key);
    }
};

/// @brief Policy trait struct detailing how keys and values are extracted and
/// stored for a `flat_hash_set`.
/// @tparam T The element type stored in the set.
template <typename T> struct SetPolicy {
    using key_type = T;
    using mapped_type = void;
    using value_type = T;
    using slot_type = T;

    /// @brief The default hashing functor used if none is provided.
    using default_hasher_type = CityHash<key_type>;
    /// @brief The default equality functor used if none is provided.
    using default_eq_type = std::equal_to<key_type>;
    /// @brief The default allocator used if none is provided.
    using default_allocator_type = std::allocator<slot_type>;

    /// @brief Extracts the key from a given slot. (Identity for a Set).
    HMM_NODISCARD static constexpr const key_type&
    key(const slot_type& slot) noexcept {
        // NOLINTNEXTLINE(bugprone-return-const-ref-from-parameter)
        return slot;
    }

    /// @brief Extracts the mutable value from a given slot.
    HMM_NODISCARD static constexpr value_type&
    value_from_slot(slot_type& slot) noexcept {
        return slot;
    }

    /// @brief Extracts the constant value from a given slot.
    HMM_NODISCARD static constexpr const value_type&
    value_from_slot(const slot_type& slot) noexcept {
        // NOLINTNEXTLINE(bugprone-return-const-ref-from-parameter)
        return slot;
    }

    /// @brief Constructs an element in-place within a slot.
    /// @tparam Alloc The allocator type.
    /// @tparam Args Forwarded argument types for construction.
    template <class Alloc, class... Args>
    static HMM_CONSTEXPR_20 void construct(Alloc& alloc, slot_type* ptr,
                                           Args&&... args) {
        std::allocator_traits<Alloc>::construct(alloc, ptr,
                                                std::forward<Args>(args)...);
    }

    /// @brief Destroys an element housed within a slot.
    /// @tparam Alloc The allocator type.
    template <class Alloc>
    static HMM_CONSTEXPR_20 void destroy(Alloc& alloc, slot_type* ptr) {
        std::allocator_traits<Alloc>::destroy(alloc, ptr);
    }
};

#if HMM_HAS_CXX_17
namespace pmr {
/// @brief An alias for a `flat_hash_set` utilizing a polymorphic memory
/// resource allocator.
/// @details This enables using custom memory arenas (like
/// `std::pmr::monotonic_buffer_resource`) to allocate set elements, which is
/// ideal for performance-sensitive, short-lived containers.
template <class Contained,
          class Hash = typename SetPolicy<Contained>::default_hasher_type,
          class Eq = typename SetPolicy<Contained>::default_eq_type>
using flat_hash_set = ::hmm::flat_hash_set<
    Contained, Hash, Eq,
    std::pmr::polymorphic_allocator<typename SetPolicy<Contained>::slot_type>>;
} // namespace pmr
#endif

} // namespace hmm

#endif // HMM_HMM_FLAT_HASH_SET_HPP
