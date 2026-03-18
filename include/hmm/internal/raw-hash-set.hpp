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

#ifndef HMM_HMM_INTERNAL_RAW_HASH_SET_HPP
#define HMM_HMM_INTERNAL_RAW_HASH_SET_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "hmm/internal/bit-mask.hpp"
#include "hmm/internal/compressed-tuple.hpp"
#include "hmm/internal/detail.hpp"
#include "hmm/internal/macros.hpp"

namespace hmm {
namespace internal {

/// @brief A wrapper union that prevents default initialization of a pointer.
///
/// This union is utilized to hold pointers that might point to uninitialized
/// or manually managed memory regions. By wrapping the pointer in a union,
/// the compiler avoids injecting implicit default initialization unless
/// explicitly requested.
///
/// @tparam T The type of the underlying object being pointed to.
template <class T>
union MaybeUninitialized {
    /// @brief Constructs the union with a provided pointer.
    /// @param ptr The pointer to initialize the union with.
    constexpr T* get() const noexcept {
        return ptr_;
    }

    /// @brief Modifies the stored pointer.
    /// @param ptr The new pointer value to store.
    HMM_CONSTEXPR_14 void set(T* ptr) noexcept {
        ptr_ = ptr;
    }

    HMM_CONSTEXPR_14 MaybeUninitialized& operator+=(ptrdiff_t diff) noexcept {
        ptr_ += diff;
        return *this;
    }

    HMM_CONSTEXPR_14 MaybeUninitialized
    operator+(ptrdiff_t diff) const noexcept {
        auto tmp = *this;
        tmp += diff;
        return tmp;
    }

    HMM_CONSTEXPR_14 MaybeUninitialized& operator-=(ptrdiff_t diff) noexcept {
        ptr_ -= diff;
        return *this;
    }

    HMM_CONSTEXPR_14 MaybeUninitialized
    operator-(ptrdiff_t diff) const noexcept {
        auto tmp = *this;
        tmp -= diff;
        return tmp;
    }

    HMM_CONSTEXPR_14 MaybeUninitialized& operator++() noexcept {
        ++ptr_;
        return *this;
    }

    HMM_CONSTEXPR_14 MaybeUninitialized& operator--() noexcept {
        --ptr_;
        return *this;
    }

    HMM_CONSTEXPR_14 MaybeUninitialized operator++(int) noexcept {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    HMM_CONSTEXPR_14 MaybeUninitialized operator--(int) noexcept {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr bool operator==(const T* that) const noexcept {
        return ptr_ == that;
    }

    constexpr bool operator!=(const T* that) const noexcept {
        return !(*this == that);
    }

    constexpr bool operator==(MaybeUninitialized that) const noexcept {
        return ptr_ == that.ptr_;
    }

    constexpr bool operator!=(MaybeUninitialized that) const noexcept {
        return !(*this == that);
    }

    T* ptr_;
};

using ctrl_t = std::int8_t;

/// @brief Manages the primary heap-allocated memory pointers for the hash set.
///
/// The hash set allocates a single contiguous block of memory. This structure
/// keeps track of the two primary regions within that block: the control bytes
/// (used for SIMD-accelerated metadata lookups) and the actual element slots.
struct HeapPtrs {
    /// @brief Gets the pointer to the array of control bytes.
    HMM_NODISCARD constexpr ctrl_t* get_ctrl() const noexcept {
        return ctrl_.get();
    }

    /// @brief Sets the pointer to the array of control bytes.
    HMM_CONSTEXPR_14 void set_ctrl(ctrl_t* ctrl) noexcept {
        ctrl_.set(ctrl);
    }

    /// @brief Gets the weakly-typed pointer to the array of value slots.
    HMM_NODISCARD constexpr void* get_slots() const noexcept {
        return slots_.get();
    }

    /// @brief Sets the weakly-typed pointer to the array of value slots.
    HMM_CONSTEXPR_14 void set_slots(void* the_slots) noexcept {
        slots_.set(the_slots);
    }

    MaybeUninitialized<ctrl_t> ctrl_{nullptr};
    MaybeUninitialized<void> slots_{nullptr};
};

/// @brief Encapsulates the runtime sizing metrics of the hash set.
struct SizeInfo {
    size_t size_ = 0;  ///< The number of active, constructed elements.

    size_t capacity_ =
        0;  ///< The total number of allocated slots (must be a power of two).
};

/// @brief Centralized state object storing policy dependencies and table
/// metadata.
///
/// Inherits from `CompressedTuple` to leverage Empty Base Class Optimization
/// (EBCO). If `Hash`, `Eq`, or `Alloc` are stateless (e.g., standard functors),
/// they consume zero bytes of memory, drastically reducing the overall
/// footprint of the container.
///
/// @tparam Hash The hashing functor type.
/// @tparam Eq The equality comparison functor type.
/// @tparam Alloc The allocator type used for memory management.
template <class Hash, class Eq, class Alloc>
struct CommonMembers : CompressedTuple<Hash, Eq, Alloc> {
    using Base = CompressedTuple<Hash, Eq, Alloc>;

    /// @brief Forwarding constructor for dependencies.
    template <
        class H, class E, class A,
        typename std::enable_if<std::is_convertible<H&&, Hash>::value &&
                                    std::is_convertible<E&&, Eq>::value &&
                                    std::is_convertible<A&&, Alloc>::value,
                                int>::type = 0>
    HMM_CONSTEXPR_20 inline CommonMembers(H&& h, E&& e, A&& a)
        : Base{static_cast<H&&>(h), static_cast<E&&>(e), static_cast<A&&>(a)},
          ptrs_{},
          size_info_{} {}

    /// @brief Default constructor. Value-initializes all sub-components.
    CommonMembers() = default;

    /// @brief Retrieves a mutable reference to the hashing functor.
    HMM_CONSTEXPR_14 Hash& get_hasher() noexcept {
        return Base::template get<0>();
    }

    /// @brief Retrieves a const reference to the hashing functor.
    constexpr const Hash& get_hasher() const noexcept {
        return Base::template get<0>();
    }

    /// @brief Retrieves a mutable reference to the equality functor.
    HMM_CONSTEXPR_14 Eq& get_eq() noexcept {
        return Base::template get<1>();
    }

    /// @brief Retrieves a const reference to the equality functor.
    constexpr const Eq& get_eq() const noexcept {
        return Base::template get<1>();
    }

    /// @brief Retrieves a mutable reference to the allocator.
    HMM_CONSTEXPR_14 Alloc& get_allocator() noexcept {
        return Base::template get<2>();
    }

    /// @brief Retrieves a const reference to the allocator.
    constexpr const Alloc& get_allocator() const noexcept {
        return Base::template get<2>();
    }

    /// @brief Retrieves the control byte array pointer.
    constexpr ctrl_t* get_ctrl() const noexcept {
        return ptrs_.get_ctrl();
    }

    /// @brief Updates the control byte array pointer.
    HMM_CONSTEXPR_14 void set_ctrl(ctrl_t* ctrl) noexcept {
        ptrs_.set_ctrl(ctrl);
    }

    /// @brief Retrieves the slot array pointer.
    constexpr void* get_slots() const noexcept {
        return ptrs_.get_slots();
    }

    /// @brief Updates the slot array pointer.
    HMM_CONSTEXPR_14 void set_slots(void* the_slots) noexcept {
        ptrs_.set_slots(the_slots);
    }

    HeapPtrs ptrs_;
    SizeInfo size_info_;
};

/// @brief Type trait to detect if a functor supports transparent heterogeneous
/// lookup.
template <typename T, typename = void>
struct HasIsTransparent : std::false_type {};

template <typename T>
struct HasIsTransparent<T,
                        typename std::enable_if<T::is_transparent::value>::type>
    : std::true_type {};

/// @brief The core SwissTable-style flat hash set implementation.
///
/// `raw_hash_set` uses open addressing with linear probing and SIMD-accelerated
/// byte-level metadata scanning. It serves as the underlying backbone for
/// both `flat_hash_set` and `flat_hash_map`.
/// Memory is allocated in a single contiguous block containing both the 1-byte
/// control group array and the tightly packed data slots array.
///
/// @tparam Policy Determines how keys and values are extracted (differentiates
/// set vs map) and provides default types.
/// @tparam TArgs Variadic pack defining [Hash, Eq, Allocator]. Falls back to
/// Policy defaults.
template <class Policy, class... TArgs>
class raw_hash_set {
   public:
    using policy_type = Policy;

    using hasher_type = typename detail::TypeAtIndexOrDefault<
        0, typename policy_type::default_hasher_type, TArgs...>::type;
    using key_equal = typename detail::TypeAtIndexOrDefault<
        1, typename policy_type::default_eq_type, TArgs...>::type;

    using key_type = typename policy_type::key_type;
    using value_type = typename policy_type::value_type;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using slot_type = typename policy_type::slot_type;

   private:
    using provided_allocator_type = typename detail::TypeAtIndexOrDefault<
        2, typename policy_type::default_allocator_type, TArgs...>::type;

   public:
    // Use a byte allocator for the unified memory block
    using byte_allocator = typename std::allocator_traits<
        provided_allocator_type>::template rebind_alloc<unsigned char>;
    using allocator_type = typename std::allocator_traits<
        provided_allocator_type>::template rebind_alloc<slot_type>;
    using slot_allocator = allocator_type;
    using slot_traits = std::allocator_traits<slot_allocator>;
    using pointer = typename slot_traits::pointer;

   private:
    using Members = CommonMembers<hasher_type, key_equal, byte_allocator>;

    static constexpr size_t kGroupWidth = Group::kWidth;

   public:
    /// @brief The underlying iterator implementation.
    /// @tparam Traits Differentiates between const and mutable iterators.
    template <typename Traits>
    class BasicIterator {
        friend raw_hash_set;
        template <typename>
        friend class iterator_impl;

       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename raw_hash_set::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer =
            typename std::conditional<Traits::is_const, const value_type*,
                                      value_type*>::type;
        using reference =
            typename std::conditional<Traits::is_const, const value_type&,
                                      value_type&>::type;

        constexpr BasicIterator() = default;

        /// @brief Implicitly converts a mutable iterator to a const iterator.
        template <typename OtherTraits,
                  typename = typename std::enable_if<(!OtherTraits::is_const ||
                                                      (Traits::is_const &&
                                                       OtherTraits::is_const)),
                                                     void>::type>
        constexpr BasicIterator(const BasicIterator<OtherTraits>& other)
            : ctrl_(other.ctrl_),
              slots_(other.slots_),
              end_ctrl_(other.end_ctrl_) {}

        HMM_NODISCARD constexpr reference operator*() const {
            return policy_type::value_from_slot(*get_slots());
        }

        HMM_NODISCARD constexpr pointer operator->() const {
            return std::addressof(operator*());
        }

        HMM_CONSTEXPR_14 BasicIterator& operator++() {
            do {
                ++ctrl_;
                ++slots_;
            } while (get_ctrl() != get_end_ctrl() && *get_ctrl() < 0);
            return *this;
        }

        HMM_CONSTEXPR_14 BasicIterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        HMM_NODISCARD constexpr bool operator==(
            const BasicIterator& b) const noexcept {
            return slots_ == b.slots_;
        }
        HMM_NODISCARD constexpr bool operator!=(
            const BasicIterator& b) const noexcept {
            return !this->operator==(b);
        }

       private:
        constexpr BasicIterator(ctrl_t* ctrl, slot_type* slot, ctrl_t* end_ctrl)
            : ctrl_{ctrl}, slots_{slot}, end_ctrl_{end_ctrl} {}

        HMM_CONSTEXPR_14 void skip_empty_or_deleted() {
            while (get_ctrl() != get_end_ctrl() && *get_ctrl() < 0) {
                ++ctrl_;
                ++slots_;
            }
        }

        constexpr ctrl_t* get_ctrl() const noexcept {
            return ctrl_.get();
        }

        constexpr slot_type* get_slots() const noexcept {
            return slots_.get();
        }

        constexpr ctrl_t* get_end_ctrl() const noexcept {
            return end_ctrl_.get();
        }

        MaybeUninitialized<ctrl_t> ctrl_;
        MaybeUninitialized<slot_type> slots_;
        MaybeUninitialized<ctrl_t> end_ctrl_;
    };

    struct NormalIteratorTraits {
        static constexpr bool is_const = false;
    };

    struct ConstIteratorTraits {
        static constexpr bool is_const = true;
    };

    using iterator = BasicIterator<NormalIteratorTraits>;
    using const_iterator = BasicIterator<ConstIteratorTraits>;

    /// @brief Returned by internal insertion preparations, containing index
    /// routing data.
    struct HMM_NODISCARD FindInfo {
        size_t index;      ///< The mapped slot index where the element exists
                           ///< or should be inserted.
        size_t full_hash;  ///< The pre-computed full 64-bit hash.
        bool found;  ///< True if the key already exists at the target index.
    };

    /// @brief Default constructs an empty hash set with no allocated memory.
    HMM_CONSTEXPR_20 raw_hash_set() = default;

    /// @brief Constructs an empty hash set using a specific allocator.
    HMM_CONSTEXPR_20 explicit raw_hash_set(const allocator_type& alloc)
        : members_(hasher_type{}, key_equal{}, byte_allocator(alloc)) {}

    /// @brief Constructs a hash set from an iterator range.
    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 raw_hash_set(
        Iter begin, Sentinel end,
        const allocator_type& alloc = allocator_type())
        : raw_hash_set(alloc) {
        insert(begin, end);
    }

    /// @brief Constructs a hash set from an initializer list.
    HMM_CONSTEXPR_20 raw_hash_set(
        std::initializer_list<value_type> init,
        const allocator_type& alloc = allocator_type())
        : raw_hash_set(init.begin(), init.end(), alloc) {}

    /// @brief Destructs the container, destroying all elements and releasing
    /// memory.
    HMM_CONSTEXPR_20 ~raw_hash_set() {
        clear_and_deallocate();
    }

    /// @brief Move-constructs the hash set, transferring ownership of the
    /// internal buffer.
    HMM_CONSTEXPR_20 raw_hash_set(raw_hash_set&& other) noexcept
        : members_(std::move(other.members_)) {
        other.members_.set_ctrl(nullptr);
        other.members_.set_slots(nullptr);
        other.members_.size_info_.capacity_ = 0;
        other.members_.size_info_.size_ = 0;
    }

    /// @brief Move-assigns the hash set, releasing old memory and transferring
    /// ownership.
    HMM_CONSTEXPR_20 raw_hash_set& operator=(raw_hash_set&& other) noexcept {
        if (this != &other) {
            clear_and_deallocate();
            members_ = std::move(other.members_);
            other.members_.set_ctrl(nullptr);
            other.members_.set_slots(nullptr);
            other.members_.size_info_.capacity_ = 0;
            other.members_.size_info_.size_ = 0;
        }
        return *this;
    }

    /// @brief Copy-constructs the hash set, duplicating elements into a new
    /// allocation.
    raw_hash_set(const raw_hash_set& other)
        : members_(other.hasher(), other.equal(),
                   std::allocator_traits<byte_allocator>::
                       select_on_container_copy_construction(
                           other.get_allocator())) {
        reserve(other.size());
        for (const auto& elem : other) {
            insert(elem);
        }
    }

    /// @brief Copy-assigns the hash set, destroying current elements and
    /// copying new ones.
    raw_hash_set& operator=(const raw_hash_set& other) {
        if (this != &other) {
            clear();
            reserve(other.size());
            for (const auto& elem : other) {
                insert(elem);
            }
        }
        return *this;
    }

    /// @brief Returns an iterator to the first active element in the container.
    HMM_NODISCARD HMM_CONSTEXPR_20 iterator begin() {
        if (empty()) {
            return end();
        }
        auto it = iterator(ctrl_ptr(), slots_ptr(), ctrl_ptr() + capacity());
        it.skip_empty_or_deleted();
        return it;
    }

    /// @brief Returns a const iterator to the first active element.
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator begin() const {
        if (empty()) {
            return end();
        }
        auto it =
            const_iterator(ctrl_ptr(), slots_ptr(), ctrl_ptr() + capacity());
        it.skip_empty_or_deleted();
        return it;
    }

    /// @brief Returns a const iterator to the first active element.
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator cbegin() const {
        return begin();
    }

    /// @brief Returns an iterator representing the end of the container.
    HMM_NODISCARD HMM_CONSTEXPR_20 iterator end() {
        return iterator(ctrl_ptr() + capacity(), slots_ptr() + capacity(),
                        ctrl_ptr() + capacity());
    }

    /// @brief Returns a const iterator representing the end of the container.
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator end() const {
        return const_iterator(ctrl_ptr() + capacity(), slots_ptr() + capacity(),
                              ctrl_ptr() + capacity());
    }

    /// @brief Returns a const iterator representing the end of the container.
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator cend() const {
        return end();
    }

    /// @brief Retrieves the number of elements currently stored in the set.
    HMM_NODISCARD constexpr size_type size() const noexcept {
        return members_.size_info_.size_;
    }

    /// @brief Checks if the container has no elements.
    HMM_NODISCARD constexpr bool empty() const noexcept {
        return members_.size_info_.size_ == 0;
    }

    /// @brief Retrieves the total number of slots allocated in the underlying
    /// buffer.
    HMM_NODISCARD constexpr size_type capacity() const noexcept {
        return members_.size_info_.capacity_;
    }

    /// @brief Destroys all elements but leaves the capacity unchanged.
    HMM_CONSTEXPR_20 void clear() {
        if (!slots_ptr()) {
            return;
        }
        clear_elements();
        std::memset(ctrl_ptr(), detail::slots::kEmpty,
                    capacity() + kGroupWidth);
        members_.size_info_.size_ = 0;
    }

    /// @brief Destroys all elements but leaves the capacity unchanged.
    std::pair<iterator, bool> insert(const value_type& value) {
        return emplace(value);
    }

    /// @brief Inserts an element into the container via move mechanics.
    std::pair<iterator, bool> insert(value_type&& value) {
        return emplace(std::move(value));
    }

    /// @brief Inserts a range of elements into the container.
    template <class InputIt>
    void insert(InputIt first, InputIt last) {
        for (; first != last; ++first) {
            emplace(*first);
        }
    }

    /// @brief Proactively resizes the container to accommodate at least `count`
    /// elements without rehashing.
    void reserve(size_type count) {
        if (count == 0) {
            return;
        }

        // capacity * 0.875 >= count
        size_type min_cap = (count * 8 + 6) / 7;
        size_type cap = 16;
        while (cap < min_cap) {
            cap <<= 1;
        }
        if (cap > capacity()) {
            rehash_and_grow(cap);
        }
    }

    /// @brief Locates an element matching the provided key.
    /// @param key The key to look for.
    /// @return An iterator to the element, or `end()` if not found.
    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 iterator find(const K& key) noexcept {
        if (empty()) {
            return end();
        }

        const auto full_hash = hasher()(key);
        const auto h2 = detail::H2(full_hash);
        size_t index =
            detail::IndexWithoutProbing(detail::H1(full_hash), capacity());

        while (true) {
            Group g = Group::Load(ctrl_ptr() + index);
            for (BitMask mask = g.Match(h2); mask; ++mask) {
                size_t probe_index =
                    (index + mask.first_index()) & (capacity() - 1);
                if (equal()(key, policy_type::key(slots_ptr()[probe_index]))) {
                    return iterator(ctrl_ptr() + probe_index,
                                    slots_ptr() + probe_index,
                                    ctrl_ptr() + capacity());
                }
            }
            if (g.MatchEmpty()) {
                return end();
            }
            index += kGroupWidth;
            if (index >= capacity()) {
                index = 0;
            }
        }
    }

    /// @brief Locates an element matching the provided key (const context).
    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator
    find(const K& key) const noexcept {
        if (empty()) {
            return end();
        }
        const auto full_hash = hasher()(key);
        const auto h2 = detail::H2(full_hash);
        size_t index =
            detail::IndexWithoutProbing(detail::H1(full_hash), capacity());

        while (true) {
            Group g = Group::Load(ctrl_ptr() + index);
            for (BitMask mask = g.Match(h2); mask; ++mask) {
                size_t probe_index =
                    (index + mask.first_index()) & (capacity() - 1);
                if (equal()(key, policy_type::key(slots_ptr()[probe_index]))) {
                    return const_iterator(ctrl_ptr() + probe_index,
                                          slots_ptr() + probe_index,
                                          ctrl_ptr() + capacity());
                }
            }
            if (g.MatchEmpty()) {
                return end();
            }
            index += kGroupWidth;
            if (index >= capacity()) {
                index = 0;
            }
        }
    }

    /// @brief Checks if an element with the exact key type exists in the
    /// container.
    HMM_NODISCARD bool contains(const key_type& key) const noexcept {
        return find(key) != end();
    }

    /// @brief Heterogeneous lookup to check if a key exists in the container.
    /// @details Only enabled via SFINAE if both the Hash and Eq types define
    /// `is_transparent`.
    template <
        typename K, typename H = hasher_type, typename E = key_equal,
        typename = typename std::enable_if<HasIsTransparent<H>::value &&
                                           HasIsTransparent<E>::value>::type>
    HMM_NODISCARD HMM_CONSTEXPR_20 bool contains(const K& key) const noexcept {
        return find(key) != end();
    }

    /// @brief Computes the target slot for an arbitrary key, determining if an
    /// insertion is required.
    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 FindInfo
    find_or_prepare_insert(const K& key) {
        const auto full_hash = hasher()(key);
        if (capacity() == 0) {
            return {0, full_hash, false};
        }

        const auto h2 = detail::H2(full_hash);
        size_t index =
            detail::IndexWithoutProbing(detail::H1(full_hash), capacity());

        while (true) {
            Group g = Group::Load(ctrl_ptr() + index);
            for (BitMask mask = g.Match(h2); mask; ++mask) {
                size_t probe_index =
                    (index + mask.first_index()) & (capacity() - 1);
                if (equal()(key, policy_type::key(slots_ptr()[probe_index]))) {
                    return {probe_index, full_hash, true};
                }
            }
            if (auto mask = g.MatchEmpty()) {
                size_t probe_index =
                    (index + mask.first_index()) & (capacity() - 1);
                return {probe_index, full_hash, false};
            }
            index += kGroupWidth;
            if (index >= capacity()) {
                index = 0;
            }
        }
    }

    /// @brief Constructs an element in-place within the table using the
    /// provided arguments.
    template <typename... Args>
    HMM_CONSTEXPR_20 std::pair<iterator, bool> emplace(Args&&... args) {
        if (needs_resize()) {
            rehash_and_grow();
        }

        slot_type temp =
            detail::construct<slot_type>(std::forward<Args>(args)...);
        const auto& key = policy_type::key(temp);

        auto info = find_or_prepare_insert(key);
        if (info.found) {
            return {iterator(ctrl_ptr() + info.index, slots_ptr() + info.index,
                             ctrl_ptr() + capacity()),
                    false};
        }

        insert_at_index(info.index, info.full_hash, std::move(temp));
        return {iterator(ctrl_ptr() + info.index, slots_ptr() + info.index,
                         ctrl_ptr() + capacity()),
                true};
    }

    /// @brief Attempts to construct an element in-place only if the provided
    /// key does not already exist.
    /// @details Critical for mapping, allowing piecewise construction where the
    /// value relies on deferred creation.
    template <class K, class... Args>
    HMM_CONSTEXPR_20 std::pair<iterator, bool> try_emplace(K&& key,
                                                           Args&&... args) {
        // LOOKUP FIRST. Do not resize yet.
        // We pass the key assuming find_or_prepare_insert takes `const K&`
        // and does not consume the r-value reference.
        auto info = find_or_prepare_insert(key);

        // CASE: FOUND
        // Return existing element. No resize needed, even if map is "full".
        if (info.found) {
            return {iterator(ctrl_ptr() + info.index, slots_ptr() + info.index,
                             ctrl_ptr() + capacity()),
                    false};
        }

        // CASE: NOT FOUND (INSERTION REQUIRED)
        // Now we check if we have space.
        if (needs_resize()) {
            rehash_and_grow();

            // The table just changed size/location. The 'info' index is now
            // invalid. We must re-calculate the hash (if needed) and find the
            // new slot.
            info = find_or_prepare_insert(key);
        }

        policy_type::construct(
            get_allocator(), &slots_ptr()[info.index], std::piecewise_construct,
            std::forward_as_tuple(std::forward<K>(key)),
            std::forward_as_tuple(std::forward<Args>(args)...));

        finish_insert(info.index, info.full_hash);

        return {iterator(ctrl_ptr() + info.index, slots_ptr() + info.index,
                         ctrl_ptr() + capacity()),
                true};
    }

    /// @brief Erases the element at the specified iterator position.
    /// @return An iterator to the element immediately following the removed
    /// element.
    HMM_CONSTEXPR_20 iterator erase(const_iterator cit) {
        size_t index = cit.get_slots() - slots_ptr();

        policy_type::destroy(get_allocator(), &slots_ptr()[index]);

        ctrl_ptr()[index] = detail::slots::kDeleted;
        if (index < kGroupWidth) {
            ctrl_ptr()[capacity() + index] = detail::slots::kDeleted;
        }

        --members_.size_info_.size_;
        auto it =
            iterator(cit.get_ctrl(), const_cast<slot_type*>(cit.get_slots()),
                     cit.get_end_ctrl());
        it.skip_empty_or_deleted();
        return it;
    }

    /// @brief Erases the element matching the provided key.
    /// @return 1 if an element was erased, 0 otherwise.
    HMM_CONSTEXPR_20 size_type erase_element(const key_type& key) {
        const auto it = find(key);
        if (it == end()) {
            return 0;
        }
        erase(it);
        return 1;
    }

    /// @brief Erases the element matching the provided key. Uses template.
    /// Providing a transparent interface.
    /// @return 1 if an element was erased, 0 otherwise.
    template <class Key>
    HMM_CONSTEXPR_20 size_type erase_element(const Key& key) {
        const auto it = find(key);
        if (it == end()) {
            return 0;
        }
        erase(it);
        return 1;
    }

    /// @brief Internal Hook: Retrieves a mutable reference to the internal size
    /// counter.
    HMM_NODISCARD constexpr size_type& size_ref() noexcept {
        return members_.size_info_.size_;
    }

    /// @brief Internal Hook: Exposes the raw control bytes pointer.
    HMM_NODISCARD ctrl_t* ctrl_ptr() const noexcept {
        return members_.get_ctrl();
    }

    /// @brief Internal Hook: Exposes the raw data slots pointer.
    HMM_NODISCARD slot_type* slots_ptr() const noexcept {
        return static_cast<slot_type*>(members_.get_slots());
    }

    /// @brief Forces the container to dynamically allocate a larger block and
    /// re-insert all items.
    HMM_CONSTEXPR_20 void rehash_and_grow() {
        size_type new_cap = (capacity() == 0) ? 16 : capacity() * 2;
        rehash_and_grow(new_cap);
    }

    /// @brief Rehashes the table and grows it to an explicit new capacity.
    /// @param new_cap The exact new capacity. Must be a power of two.
    HMM_CONSTEXPR_20 void rehash_and_grow(const size_type new_cap) {
        auto old_ctrl = ctrl_ptr();
        auto old_slots = slots_ptr();
        auto old_cap = capacity();

        allocate_storage(new_cap);
        std::memset(ctrl_ptr(), detail::slots::kEmpty, new_cap + kGroupWidth);
        members_.size_info_.size_ = 0;

        if (old_slots) {
            for (size_t i = 0; i < old_cap; ++i) {
                if (old_ctrl[i] >= 0) {
                    auto& val = old_slots[i];
                    auto info = find_or_prepare_insert(policy_type::key(val));
                    policy_type::construct(get_allocator(),
                                           &slots_ptr()[info.index],
                                           std::move(val));
                    finish_insert(info.index, info.full_hash);
                    policy_type::destroy(get_allocator(), &old_slots[i]);
                }
            }
            deallocate_storage(old_ctrl, old_cap);
        }
    }

    /// @brief Internal Hook: Given a guaranteed index and hash, constructs the
    /// element into the slot array.
    void insert_at_index(size_t index, size_t full_hash, slot_type&& val) {
        policy_type::construct(get_allocator(), &slots_ptr()[index],
                               std::move(val));
        finish_insert(index, full_hash);
    }

    /// @brief Commits an insertion by updating the control byte metadata array.
    void finish_insert(size_t index, size_t full_hash) {
        auto h2 = detail::H2(full_hash);
        ctrl_ptr()[index] = h2;
        if (index < kGroupWidth) {
            ctrl_ptr()[capacity() + index] = h2;
        }
        ++members_.size_info_.size_;
    }

    /// @brief Acquires memory via the allocator for a specified capacity.
    /// @details Safely computes alignments and buffer sizes to house both
    /// metadata bytes and strictly aligned elements in one allocation block.
    HMM_CONSTEXPR_20 void allocate_storage(const size_type cap) {
        size_t ctrl_size = cap + kGroupWidth;
        size_t slot_align = alignof(slot_type);
        size_t slot_offset = (ctrl_size + slot_align - 1) & ~(slot_align - 1);
        size_t total_bytes = slot_offset + cap * sizeof(slot_type);

        unsigned char* ptr = std::allocator_traits<byte_allocator>::allocate(
            get_allocator(), total_bytes);

        members_.set_ctrl(reinterpret_cast<ctrl_t*>(ptr));
        members_.set_slots(reinterpret_cast<slot_type*>(ptr + slot_offset));
        members_.size_info_.capacity_ = cap;
    }

    /// @brief Releases the memory block currently owned by the set back to the
    /// allocator.
    HMM_CONSTEXPR_20 void deallocate_storage(ctrl_t* ctrl_pointer,
                                             const size_type cap) {
        size_t ctrl_size = cap + kGroupWidth;
        size_t slot_align = alignof(slot_type);
        size_t slot_offset = (ctrl_size + slot_align - 1) & ~(slot_align - 1);
        size_t total_bytes = slot_offset + cap * sizeof(slot_type);

        std::allocator_traits<byte_allocator>::deallocate(
            get_allocator(), reinterpret_cast<unsigned char*>(ctrl_pointer),
            total_bytes);
    }

    /// @brief Invokes destructors on all actively tracked elements using the
    /// allocator traits.
    HMM_CONSTEXPR_20 void clear_elements() {
        for (size_t i = 0; i < capacity(); ++i) {
            if (ctrl_ptr()[i] >= 0) {
                policy_type::destroy(get_allocator(), &slots_ptr()[i]);
            }
        }
    }

    /// @brief Synthesizes `clear_elements` and `deallocate_storage`, fully
    /// resetting the hash set to an empty, zero-capacity state.
    HMM_CONSTEXPR_20 void clear_and_deallocate() {
        if (!ctrl_ptr()) {
            return;
        }
        clear_elements();
        deallocate_storage(ctrl_ptr(), capacity());
        members_.set_ctrl(nullptr);
        members_.set_slots(nullptr);
        members_.size_info_.capacity_ = 0;
        members_.size_info_.size_ = 0;
    }

    /// @brief Computes if the container's load factor exceeds the threshold
    /// triggering a resize (7/8).
    HMM_NODISCARD constexpr bool needs_resize() const noexcept {
        return capacity() == 0 || size() * 8 > capacity() * 7;
    }

    /// @brief Internal Hook: Retrieves the hashing functor.
    HMM_NODISCARD HMM_CONSTEXPR_14 hasher_type& hasher() noexcept {
        return members_.get_hasher();
    }

    /// @brief Internal Hook: Retrieves the equality functor.
    HMM_NODISCARD HMM_CONSTEXPR_14 key_equal& equal() noexcept {
        return members_.get_eq();
    }

    HMM_NODISCARD HMM_CONSTEXPR_14 byte_allocator& get_allocator() noexcept {
        return members_.get_allocator();
    }
    HMM_NODISCARD HMM_CONSTEXPR_14 const byte_allocator& get_allocator()
        const noexcept {
        return members_.get_allocator();
    }

    /// @brief Internal Hook: Retrieves the hashing functor (const context).
    HMM_NODISCARD constexpr const hasher_type& hasher() const noexcept {
        return members_.get_hasher();
    }

    /// @brief Internal Hook: Retrieves the equality functor (const context).
    HMM_NODISCARD constexpr const key_equal& equal() const noexcept {
        return members_.get_eq();
    }

    Members members_;
};

}  // namespace internal
}  // namespace hmm

#endif  // HMM_HMM_INTERNAL_RAW_HASH_SET_HPP
