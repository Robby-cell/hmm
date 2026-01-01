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
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "hmm/internal/bit-mask.hpp"
#include "hmm/internal/detail.hpp"
#include "hmm/internal/macros.hpp"

namespace hmm {
namespace internal {

// Trait: Checks if Hash/Equal have is_transparent
template <typename T, typename = void>
struct HasIsTransparent : std::false_type {};
template <typename T>
struct HasIsTransparent<T,
                        typename std::enable_if<T::is_transparent::value>::type>
    : std::true_type {};

template <class Policy, class Hash, class Eq, class Alloc>
class raw_hash_set {
   public:
    using policy_type = Policy;
    using hasher_type = Hash;
    using key_equal = Eq;
    using key_type = typename policy_type::key_type;
    using value_type = typename policy_type::value_type;
    using size_type = std::size_t;
    using slot_type = typename policy_type::slot_type;

    // Use a byte allocator for the unified memory block
    using byte_allocator = typename std::allocator_traits<
        Alloc>::template rebind_alloc<unsigned char>;
    using allocator_type =
        typename std::allocator_traits<Alloc>::template rebind_alloc<slot_type>;
    using slot_allocator = allocator_type;
    using slot_traits = std::allocator_traits<slot_allocator>;
    using pointer = typename slot_traits::pointer;

   private:
    using Control = std::int8_t;
    static constexpr size_t kGroupWidth = Group::kWidth;

   public:
    template <typename Value>
    class iterator_impl {
        friend raw_hash_set;
        template <typename>
        friend class iterator_impl;

       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename raw_hash_set::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        constexpr iterator_impl() = default;

        // Conversion Constructor (Mutable -> Const)
        template <typename OtherValue, typename = typename std::enable_if<
                                           (std::is_const<Value>::value &&
                                            !std::is_const<OtherValue>::value),
                                           void>::type>
        constexpr iterator_impl(const iterator_impl<OtherValue>& other)
            : ctrl_(other.ctrl_),
              slot_(other.slot_),
              end_ctrl_(other.end_ctrl_) {}

        HMM_NODISCARD constexpr reference operator*() const {
            return policy_type::value_from_slot(*slot_);
        }
        HMM_NODISCARD constexpr pointer operator->() const {
            return std::addressof(operator*());
        }

        HMM_CONSTEXPR_14 iterator_impl& operator++() {
            do {
                ++ctrl_;
                ++slot_;
            } while (ctrl_ != end_ctrl_ && *ctrl_ < 0);
            return *this;
        }

        HMM_CONSTEXPR_14 iterator_impl operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        HMM_NODISCARD constexpr bool operator==(
            const iterator_impl& b) const noexcept {
            return slot_ == b.slot_;
        }
        HMM_NODISCARD constexpr bool operator!=(
            const iterator_impl& b) const noexcept {
            return !this->operator==(b);
        }

       private:
        constexpr iterator_impl(Control* ctrl, slot_type* slot,
                                Control* end_ctrl)
            : ctrl_(ctrl), slot_(slot), end_ctrl_(end_ctrl) {}

        HMM_CONSTEXPR_14 void skip_empty_or_deleted() {
            while (ctrl_ != end_ctrl_ && *ctrl_ < 0) {
                ++ctrl_;
                ++slot_;
            }
        }

        Control* ctrl_{nullptr};
        slot_type* slot_{nullptr};
        Control* end_ctrl_{nullptr};
    };

    using iterator = iterator_impl<value_type>;
    using const_iterator = iterator_impl<const value_type>;

    struct HMM_NODISCARD FindInfo {
        std::size_t index;
        std::size_t full_hash;
        bool found;
    };

    HMM_CONSTEXPR_20 raw_hash_set() = default;

    HMM_CONSTEXPR_20 explicit raw_hash_set(const allocator_type& alloc)
        : members_(Hash{}, Eq{}, byte_allocator(alloc)) {}

    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 raw_hash_set(
        Iter begin, Sentinel end,
        const allocator_type& alloc = allocator_type())
        : raw_hash_set(alloc) {
        insert(begin, end);
    }

    HMM_CONSTEXPR_20 raw_hash_set(
        std::initializer_list<value_type> init,
        const allocator_type& alloc = allocator_type())
        : raw_hash_set(init.begin(), init.end(), alloc) {}

    HMM_CONSTEXPR_20 ~raw_hash_set() {
        clear_and_deallocate();
    }

    HMM_CONSTEXPR_20 raw_hash_set(raw_hash_set&& other) noexcept
        : members_(std::move(other.members_)),
          ctrl_(detail::exchange(other.ctrl_, nullptr)),
          slots_(detail::exchange(other.slots_, nullptr)),
          capacity_(detail::exchange(other.capacity_, 0)),
          size_(detail::exchange(other.size_, 0)) {}

    HMM_CONSTEXPR_20 raw_hash_set& operator=(raw_hash_set&& other) noexcept {
        if (this != &other) {
            clear_and_deallocate();
            members_ = std::move(other.members_);
            ctrl_ = detail::exchange(other.ctrl_, nullptr);
            slots_ = detail::exchange(other.slots_, nullptr);
            capacity_ = detail::exchange(other.capacity_, 0);
            size_ = detail::exchange(other.size_, 0);
        }
        return *this;
    }

    raw_hash_set(const raw_hash_set& other)
        : members_(
              other.hasher(), other.equal(),
              std::allocator_traits<byte_allocator>::
                  select_on_container_copy_construction(other.byte_alloc())) {
        reserve(other.size());
        for (const auto& elem : other) {
            insert(elem);
        }
    }

    raw_hash_set& operator=(const raw_hash_set& other) {
        if (this != &other) {
            clear();
            // Note: Standard requires checking
            // propagate_on_container_copy_assignment here.
            reserve(other.size());
            for (const auto& elem : other) {
                insert(elem);
            }
        }
        return *this;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 iterator begin() {
        if (empty()) {
            return end();
        }
        auto it = iterator(ctrl_, slots_, ctrl_ + capacity());
        it.skip_empty_or_deleted();
        return it;
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator begin() const {
        if (empty()) {
            return end();
        }
        auto it = const_iterator(ctrl_, slots_, ctrl_ + capacity());
        it.skip_empty_or_deleted();
        return it;
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator cbegin() const {
        return begin();
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 iterator end() {
        return iterator(ctrl_ + capacity(), slots_ + capacity(),
                        ctrl_ + capacity());
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator end() const {
        return const_iterator(ctrl_ + capacity(), slots_ + capacity(),
                              ctrl_ + capacity());
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator cend() const {
        return end();
    }

    HMM_NODISCARD constexpr size_type size() const noexcept {
        return size_;
    }
    HMM_NODISCARD constexpr bool empty() const noexcept {
        return size_ == 0;
    }
    HMM_NODISCARD constexpr size_type capacity() const noexcept {
        return capacity_;
    }

    HMM_CONSTEXPR_20 void clear() {
        if (!slots_) {
            return;
        }
        clear_elements();
        std::memset(ctrl_, detail::slots::kEmpty, capacity() + kGroupWidth);
        size_ = 0;
    }

    // Insert Overloads
    std::pair<iterator, bool> insert(const value_type& value) {
        return emplace(value);
    }
    std::pair<iterator, bool> insert(value_type&& value) {
        return emplace(std::move(value));
    }
    template <class InputIt>
    void insert(InputIt first, InputIt last) {
        for (; first != last; ++first) {
            emplace(*first);
        }
    }
    void insert(std::initializer_list<value_type> ilist) {
        insert(ilist.begin(), ilist.end());
    }

    // Reserve
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

    // Lookup
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
            Group g = Group::Load(ctrl_ + index);
            for (BitMask mask = g.Match(h2); mask; ++mask) {
                size_t probe_index =
                    (index + mask.first_index()) & (capacity() - 1);
                if (equal()(key, policy_type::key(slots_[probe_index]))) {
                    return iterator(ctrl_ + probe_index, slots_ + probe_index,
                                    ctrl_ + capacity());
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
            Group g = Group::Load(ctrl_ + index);
            for (BitMask mask = g.Match(h2); mask; ++mask) {
                size_t probe_index =
                    (index + mask.first_index()) & (capacity() - 1);
                if (equal()(key, policy_type::key(slots_[probe_index]))) {
                    return const_iterator(ctrl_ + probe_index,
                                          slots_ + probe_index,
                                          ctrl_ + capacity());
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

    // Standard Contains (allows implicit conversions)
    HMM_NODISCARD bool contains(const key_type& key) const noexcept {
        return find(key) != end();
    }

    // Transparent Contains
    template <
        typename K, typename H = Hash, typename E = Eq,
        typename = typename std::enable_if<HasIsTransparent<H>::value &&
                                           HasIsTransparent<E>::value>::type>
    HMM_NODISCARD HMM_CONSTEXPR_20 bool contains(const K& key) const noexcept {
        return find(key) != end();
    }

    // Insertion & Emplacement
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
            Group g = Group::Load(ctrl_ + index);
            for (BitMask mask = g.Match(h2); mask; ++mask) {
                size_t probe_index =
                    (index + mask.first_index()) & (capacity() - 1);
                if (equal()(key, policy_type::key(slots_[probe_index]))) {
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
            return {iterator(ctrl_ + info.index, slots_ + info.index,
                             ctrl_ + capacity()),
                    false};
        }

        insert_at_index(info.index, info.full_hash, std::move(temp));
        return {iterator(ctrl_ + info.index, slots_ + info.index,
                         ctrl_ + capacity()),
                true};
    }

    template <class K, class... Args>
    HMM_CONSTEXPR_20 std::pair<iterator, bool> try_emplace(K&& key,
                                                           Args&&... args) {
        if (needs_resize()) {
            rehash_and_grow();
        }

        auto info = find_or_prepare_insert(key);
        if (info.found) {
            return {iterator(ctrl_ + info.index, slots_ + info.index,
                             ctrl_ + capacity()),
                    false};
        }

        using ReboundAlloc = typename std::allocator_traits<
            allocator_type>::template rebind_alloc<slot_type>;
        ReboundAlloc alloc(byte_alloc());

        policy_type::construct(
            alloc, &slots_[info.index], std::piecewise_construct,
            std::forward_as_tuple(std::forward<K>(key)),
            std::forward_as_tuple(std::forward<Args>(args)...));

        finish_insert(info.index, info.full_hash);
        return {iterator(ctrl_ + info.index, slots_ + info.index,
                         ctrl_ + capacity()),
                true};
    }

    // Erasure
    HMM_CONSTEXPR_20 iterator erase(const_iterator cit) {
        std::size_t index = cit.slot_ - slots_;

        using ReboundAlloc = typename std::allocator_traits<
            allocator_type>::template rebind_alloc<slot_type>;
        ReboundAlloc alloc(byte_alloc());
        policy_type::destroy(alloc, &slots_[index]);

        ctrl_[index] = detail::slots::kDeleted;
        if (index < kGroupWidth) {
            ctrl_[capacity() + index] = detail::slots::kDeleted;
        }

        --size_;
        auto it = iterator(cit.ctrl_, const_cast<slot_type*>(cit.slot_),
                           cit.end_ctrl_);
        it.skip_empty_or_deleted();
        return it;
    }

    HMM_CONSTEXPR_20 size_type erase_element(const key_type& key) {
        const auto it = find(key);
        if (it == end()) {
            return 0;
        }
        erase(it);
        return 1;
    }

    // Internals
    HMM_CONSTEXPR_20 void rehash_and_grow() {
        size_type new_cap = (capacity() == 0) ? 16 : capacity() * 2;
        rehash_and_grow(new_cap);
    }

    HMM_CONSTEXPR_20 void rehash_and_grow(const size_type new_cap) {
        auto old_ctrl = ctrl_;
        auto old_slots = slots_;
        auto old_cap = capacity();

        allocate_storage(new_cap);
        std::memset(ctrl_, detail::slots::kEmpty, new_cap + kGroupWidth);
        size_ = 0;

        if (old_slots) {
            using ReboundAlloc = typename std::allocator_traits<
                allocator_type>::template rebind_alloc<slot_type>;
            ReboundAlloc alloc(byte_alloc());

            for (size_t i = 0; i < old_cap; ++i) {
                if (old_ctrl[i] >= 0) {
                    auto& val = old_slots[i];
                    auto info = find_or_prepare_insert(policy_type::key(val));
                    policy_type::construct(alloc, &slots_[info.index],
                                           std::move(val));
                    finish_insert(info.index, info.full_hash);
                    policy_type::destroy(alloc, &old_slots[i]);
                }
            }
            deallocate_storage(old_ctrl, old_cap);
        }
    }

    void insert_at_index(size_t index, size_t full_hash, slot_type&& val) {
        using ReboundAlloc = typename std::allocator_traits<
            allocator_type>::template rebind_alloc<slot_type>;
        ReboundAlloc alloc(byte_alloc());
        policy_type::construct(alloc, &slots_[index], std::move(val));
        finish_insert(index, full_hash);
    }

    void finish_insert(size_t index, size_t full_hash) {
        auto h2 = detail::H2(full_hash);
        ctrl_[index] = h2;
        if (index < kGroupWidth) {
            ctrl_[capacity() + index] = h2;
        }
        ++size_;
    }

    HMM_CONSTEXPR_20 void allocate_storage(const size_type cap) {
        size_t ctrl_size = cap + kGroupWidth;
        size_t slot_align = alignof(slot_type);
        size_t slot_offset = (ctrl_size + slot_align - 1) & ~(slot_align - 1);
        size_t total_bytes = slot_offset + cap * sizeof(slot_type);

        unsigned char* ptr = std::allocator_traits<byte_allocator>::allocate(
            byte_alloc(), total_bytes);

        ctrl_ = reinterpret_cast<Control*>(ptr);
        slots_ = reinterpret_cast<slot_type*>(ptr + slot_offset);
        capacity_ = cap;
    }

    HMM_CONSTEXPR_20 void deallocate_storage(Control* ctrl_ptr,
                                             const size_type cap) {
        size_t ctrl_size = cap + kGroupWidth;
        size_t slot_align = alignof(slot_type);
        size_t slot_offset = (ctrl_size + slot_align - 1) & ~(slot_align - 1);
        size_t total_bytes = slot_offset + cap * sizeof(slot_type);

        std::allocator_traits<byte_allocator>::deallocate(
            byte_alloc(), reinterpret_cast<unsigned char*>(ctrl_ptr),
            total_bytes);
    }

    HMM_CONSTEXPR_20 void clear_elements() {
        using ReboundAlloc = typename std::allocator_traits<
            allocator_type>::template rebind_alloc<slot_type>;
        ReboundAlloc alloc(byte_alloc());
        for (std::size_t i = 0; i < capacity_; ++i) {
            if (ctrl_[i] >= 0) {
                policy_type::destroy(alloc, &slots_[i]);
            }
        }
    }

    HMM_CONSTEXPR_20 void clear_and_deallocate() {
        if (!ctrl_) {
            return;
        }
        clear_elements();
        deallocate_storage(ctrl_, capacity_);
        ctrl_ = nullptr;
        slots_ = nullptr;
        capacity_ = 0;
        size_ = 0;
    }

    HMM_NODISCARD constexpr bool needs_resize() const noexcept {
        return capacity() == 0 || size() * 8 > capacity() * 7;
    }

    HMM_NODISCARD HMM_CONSTEXPR_14 Hash& hasher() noexcept {
        return members_.template get<0, Hash>();
    }
    HMM_NODISCARD HMM_CONSTEXPR_14 Eq& equal() noexcept {
        return members_.template get<1, Eq>();
    }

    HMM_NODISCARD HMM_CONSTEXPR_14 byte_allocator& byte_alloc() noexcept {
        return members_.template get<2, byte_allocator>();
    }
    HMM_NODISCARD HMM_CONSTEXPR_14 const byte_allocator& byte_alloc()
        const noexcept {
        return members_.template get<2, byte_allocator>();
    }

    HMM_NODISCARD constexpr const Hash& hasher() const noexcept {
        return members_.template get<0, Hash>();
    }
    HMM_NODISCARD constexpr const Eq& equal() const noexcept {
        return members_.template get<1, Eq>();
    }

    detail::CompressedTuple<Hash, Eq, byte_allocator> members_;
    Control* ctrl_{nullptr};
    slot_type* slots_{nullptr};
    size_type capacity_{0};
    size_type size_{0};
};

}  // namespace internal
}  // namespace hmm

#endif  // HMM_HMM_INTERNAL_RAW_HASH_SET_HPP
