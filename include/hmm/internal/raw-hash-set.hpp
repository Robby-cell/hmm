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

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

#include "hmm/internal/detail.hpp"
#include "hmm/internal/macros.hpp"

namespace hmm {
namespace internal {

template <class Policy, class Hash, class Eq, class Alloc>
class raw_hash_set {
   protected:
    using policy_type = Policy;
    using hasher_type = Hash;
    using key_equal = Eq;
    using key_type = typename policy_type::key_type;
    using value_type = typename policy_type::value_type;
    using size_type = std::size_t;

    using slot_type = typename policy_type::slot_type;
    using slot_allocator =
        typename std::allocator_traits<Alloc>::template rebind_alloc<slot_type>;
    using slot_traits = std::allocator_traits<slot_allocator>;
    using pointer = typename slot_traits::pointer;
    using allocator_type = slot_allocator;

    using Control = std::int8_t;
    using Slot = slot_type;

    template <typename Value>
    class HMM_NODISCARD iterator_impl {
        friend raw_hash_set;

       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename raw_hash_set::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;

        constexpr iterator_impl() = default;

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
            ++ctrl_;
            ++slot_;
            skip_empty_or_deleted();
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
        constexpr iterator_impl(Control* ctrl, Slot* slot, Control* end_ctrl)
            : ctrl_(ctrl), slot_(slot), end_ctrl_(end_ctrl) {}

        HMM_CONSTEXPR_14 void skip_empty_or_deleted() {
            while (ctrl_ != end_ctrl_ && *ctrl_ < 0) {
                ++ctrl_;
                ++slot_;
            }
        }

        Control* ctrl_{nullptr};
        Slot* slot_{nullptr};
        Control* end_ctrl_{nullptr};
    };

    template <typename Value>
    class HMM_NODISCARD const_iterator_impl {
        friend raw_hash_set;

       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const typename raw_hash_set::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        constexpr const_iterator_impl() = default;

        constexpr const_iterator_impl(
            const iterator_impl<typename raw_hash_set::value_type>& other)
            : ctrl_(other.ctrl_),
              slot_(other.slot_),
              end_ctrl_(other.end_ctrl_) {}

        HMM_NODISCARD constexpr reference operator*() const {
            return policy_type::value_from_slot(*slot_);
        }

        HMM_NODISCARD constexpr pointer operator->() const {
            return std::addressof(operator*());
        }

        HMM_CONSTEXPR_14 const_iterator_impl& operator++() {
            ++ctrl_;
            ++slot_;
            skip_empty_or_deleted();
            return *this;
        }

        HMM_CONSTEXPR_14 const_iterator_impl operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        HMM_NODISCARD constexpr bool operator==(
            const const_iterator_impl& b) const noexcept {
            return slot_ == b.slot_;
        }
        HMM_NODISCARD constexpr bool operator!=(
            const const_iterator_impl& b) const noexcept {
            return !this->operator==(b);
        }

       private:
        constexpr const_iterator_impl(Control* ctrl, const Slot* slot,
                                      Control* end_ctrl)
            : ctrl_(ctrl),
              slot_(const_cast<Slot*>(slot)),
              end_ctrl_(end_ctrl) {}

        HMM_CONSTEXPR_14 void skip_empty_or_deleted() {
            while (ctrl_ != end_ctrl_ && *ctrl_ < 0) {
                ++ctrl_;
                ++slot_;
            }
        }

        Control* ctrl_{nullptr};
        Slot* slot_{nullptr};
        Control* end_ctrl_{nullptr};
    };

    using iterator = iterator_impl<value_type>;
    using const_iterator = const_iterator_impl<const value_type>;

    struct HMM_NODISCARD FindInfo {
        std::size_t index;
        bool found;
    };

    HMM_CONSTEXPR_20 raw_hash_set() = default;

    HMM_CONSTEXPR_20 raw_hash_set(
        std::initializer_list<slot_type> initial,
        const slot_allocator& alloc = slot_allocator())
        : raw_hash_set(initial.begin(), initial.end(), alloc) {}

    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 raw_hash_set(
        Iter begin, Sentinel end,
        const slot_allocator& alloc = slot_allocator())
        : raw_hash_set(alloc) {
        for (; begin != end; ++begin) {
            insert(*begin);
        }
    }

    HMM_CONSTEXPR_20 explicit raw_hash_set(const slot_allocator& alloc)
        : members_(Hash{}, Eq{}, alloc, CtrlAllocator{}) {}

    HMM_CONSTEXPR_20 ~raw_hash_set() {
        clear_and_deallocate();
    }

    HMM_CONSTEXPR_20 raw_hash_set(raw_hash_set&& other) noexcept
        : members_(std::move(other.members_)),
          ctrl_(detail::exchange(other.ctrl_, nullptr)),
          slots_(detail::exchange(other.slots_, nullptr)),
          capacity_(detail::exchange(other.capacity_, 0)),
          size_(detail::exchange(other.size_, 0)) {}

    HMM_CONSTEXPR_20 raw_hash_set(const raw_hash_set& other) noexcept
        : raw_hash_set(other.begin(), other.end()) {}

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

    HMM_CONSTEXPR_20 raw_hash_set& operator=(
        const raw_hash_set& other) noexcept {
        if (this != &other) {
            clear();
            insert(other.begin(), other.end());
        }
        return *this;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 iterator begin() {
        auto it = iterator(ctrl_, slots_, ctrl_ + capacity());
        it.skip_empty_or_deleted();
        return it;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator begin() const {
        return cbegin();
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator cbegin() const {
        auto it = const_iterator(ctrl_, slots_, ctrl_ + capacity());
        it.skip_empty_or_deleted();
        return it;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 iterator end() {
        return iterator(ctrl_ + capacity(), slots_ + capacity(),
                        ctrl_ + capacity());
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator end() const {
        return cend();
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator cend() const {
        return const_iterator(ctrl_ + capacity(), slots_ + capacity(),
                              ctrl_ + capacity());
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
        std::fill(ctrl_, ctrl_ + capacity(), detail::slots::kEmpty);
        size_ = 0;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 FindInfo
    find_or_prepare_insert(const key_type& key) {
        if (capacity() == 0) {
            return {0, false};
        }
        const auto full_hash = hasher()(key);
        const auto h1 = detail::H1(full_hash);
        const auto pos = detail::H2(full_hash, capacity());
        std::size_t first_deleted = -1;

        for (std::size_t i = 0; i < capacity(); ++i) {
            const std::size_t index = (pos + i) & (capacity() - 1);
            if (ctrl_[index] == detail::slots::kEmpty) {
                return {(first_deleted != -1) ? first_deleted : index, false};
            }
            if (ctrl_[index] == detail::slots::kDeleted) {
                if (first_deleted == -1) {
                    first_deleted = index;
                }
            } else if (ctrl_[index] == h1 &&
                       equal()(key, policy_type::key(slots_[index]))) {
                return {index, true};
            }
        }
        return {first_deleted, false};
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 iterator find(const key_type& key) noexcept {
        if (empty()) {
            return end();
        }
        const auto full_hash = hasher()(key);
        const auto h1 = detail::H1(full_hash);
        const auto pos = detail::H2(full_hash, capacity());

        for (std::size_t i = 0; i < capacity(); ++i) {
            const std::size_t index = (pos + i) & (capacity() - 1);
            if (ctrl_[index] == h1 &&
                equal()(key, policy_type::key(slots_[index]))) {
                return iterator(ctrl_ + index, slots_ + index,
                                ctrl_ + capacity());
            }
            if (ctrl_[index] == detail::slots::kEmpty) {
                return end();
            }
        }
        return end();
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator
    find(const key_type& key) const noexcept {
        if (empty()) {
            return end();
        }
        const auto full_hash = hasher()(key);
        const auto h1 = detail::H1(full_hash);
        const auto pos = detail::H2(full_hash, capacity());

        for (std::size_t i = 0; i < capacity(); ++i) {
            const std::size_t index = (pos + i) & (capacity() - 1);
            if (ctrl_[index] == h1 &&
                equal()(key, policy_type::key(slots_[index]))) {
                return const_iterator(ctrl_ + index, slots_ + index,
                                      ctrl_ + capacity());
            }
            if (ctrl_[index] == detail::slots::kEmpty) {
                return end();
            }
        }
        return end();
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 bool contains(
        const key_type& key) const noexcept {
        return find(key) != end();
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 iterator erase(const_iterator cit) {
        std::size_t index = cit.slot_ - slots_;
        policy_type::destroy(slot_alloc(), &slots_[index]);
        ctrl_[index] = detail::slots::kDeleted;
        --size_;
        auto it =
            iterator(cit.ctrl_, const_cast<pointer>(cit.slot_), cit.end_ctrl_);
        it.skip_empty_or_deleted();
        return it;
    }

    HMM_CONSTEXPR_20 void erase_at(size_type index) {
        policy_type::destroy(slot_alloc(), &slots_[index]);
        ctrl_[index] = detail::slots::kDeleted;
        --size_;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 size_type
    erase_element(const key_type& key) {
        const auto it = find(key);
        if (it == end()) {
            return 0;
        }
        erase(it);
        return 1;
    }

    HMM_CONSTEXPR_20 void rehash_and_grow() {
        const auto old_capacity = capacity();
        const size_type new_cap = (old_capacity == 0) ? 16 : old_capacity * 2;
        return rehash_and_grow(new_cap);
    }

    HMM_CONSTEXPR_20 void rehash_and_grow(const size_type new_cap) {
        HMM_ASSERT(new_cap > capacity());

        const auto old_capacity = capacity();
        const auto old_ctrl = ctrl_;
        const auto old_slots = slots_;

        allocate_storage(new_cap);
        std::fill(ctrl_, ctrl_ + capacity(), detail::slots::kEmpty);

        size_ = 0;
        if (!old_slots) {
            return;
        }
        for (std::size_t i = 0; i < old_capacity; ++i) {
            if (old_ctrl[i] >= 0) {
                const auto& key = policy_type::key(old_slots[i]);
                auto info = find_or_prepare_insert(key);
                ctrl_[info.index] = detail::H1(hasher()(key));
                policy_type::construct(slot_alloc(), &slots_[info.index],
                                       std::move(old_slots[i]));
                policy_type::destroy(slot_alloc(), &old_slots[i]);
                ++size_;
            }
        }

        deallocate_storage(old_ctrl, old_slots, old_capacity);
    }

    HMM_CONSTEXPR_20 void reserve(const size_type capacity) {
        if (capacity) {
            rehash_and_grow(capacity + this->capacity());
        }
    }

    HMM_CONSTEXPR_20 void insert(const slot_type& value) {
        emplace(value);
    }

    HMM_CONSTEXPR_20 void insert(slot_type&& value) {
        emplace(std::move(value));
    }

    HMM_CONSTEXPR_20 void insert(std::initializer_list<slot_type> values) {
        insert(values.begin(), values.end());
    }

    // HMM_CONSTEXPR_20 void insert(const value_type& value) {
    //     emplace(value);
    // }

    // HMM_CONSTEXPR_20 void insert(value_type&& value) {
    //     emplace(std::move(value));
    // }

    // HMM_CONSTEXPR_20 void insert(std::initializer_list<value_type> values) {
    //     insert(values.begin(), values.end());
    // }

    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 void insert(Iter begin, Sentinel end) {
        for (; begin != end; ++begin) {
            emplace(*begin);
        }
    }

    template <typename... Args>
    HMM_CONSTEXPR_20 std::pair<iterator, bool> emplace(Args&&... args) {
        if (needs_resize()) {
            rehash_and_grow();
        }

        // slot_type tmp(std::forward<Args>(args)...);
        auto tmp = detail::construct<slot_type>(std::forward<Args>(args)...);

        const auto info = find_or_prepare_insert(policy_type::key(tmp));
        if (info.found) {
            return {iterator(ctrl_ + info.index, slots_ + info.index,
                             ctrl_ + capacity()),
                    false};
        }
        const auto& key = policy_type::key(tmp);
        const auto full_hash = hasher()(key);
        ctrl_[info.index] = detail::H1(full_hash);
        policy_type::construct(slot_alloc(), &slots_[info.index],
                               std::move(tmp));
        ++size_;
        return {iterator(ctrl_ + info.index, slots_ + info.index,
                         ctrl_ + capacity()),
                true};
    }

    HMM_NODISCARD HMM_CONSTEXPR_14 Control*& ctrl_ptr() {
        return ctrl_;
    }
    HMM_NODISCARD HMM_CONSTEXPR_14 Slot*& slots_ptr() {
        return slots_;
    }
    HMM_NODISCARD HMM_CONSTEXPR_14 size_type& capacity_ref() {
        return capacity_;
    }
    HMM_NODISCARD HMM_CONSTEXPR_14 size_type& size_ref() {
        return size_;
    }

    using CtrlAllocator = typename slot_traits::template rebind_alloc<Control>;
    using CtrlTraits = std::allocator_traits<CtrlAllocator>;

    HMM_NODISCARD HMM_CONSTEXPR_14 Hash& hasher() noexcept {
        return members_.template get<0, Hash>();
    }
    HMM_NODISCARD HMM_CONSTEXPR_14 Eq& equal() noexcept {
        return members_.template get<1, Eq>();
    }
    HMM_NODISCARD HMM_CONSTEXPR_14 slot_allocator& slot_alloc() noexcept {
        return members_.template get<2, slot_allocator>();
    }
    HMM_NODISCARD HMM_CONSTEXPR_14 CtrlAllocator& ctrl_alloc() noexcept {
        return members_.template get<3, CtrlAllocator>();
    }
    HMM_NODISCARD constexpr const Hash& hasher() const noexcept {
        return members_.template get<0, Hash>();
    }
    HMM_NODISCARD constexpr const Eq& equal() const noexcept {
        return members_.template get<1, Eq>();
    }
    HMM_NODISCARD constexpr const slot_allocator& slot_alloc() const noexcept {
        return members_.template get<2, slot_allocator>();
    }
    HMM_NODISCARD constexpr const CtrlAllocator& ctrl_alloc() const noexcept {
        return members_.template get<3, CtrlAllocator>();
    }

    HMM_NODISCARD constexpr bool needs_resize() const {
        return capacity() == 0 || size() * 8 > capacity() * 7;
    }

    HMM_CONSTEXPR_20 void allocate_storage(const size_type cap) {
        auto&& control_allocator = ctrl_alloc();
        ctrl_ = CtrlTraits::allocate(control_allocator, cap);
        slots_ = slot_traits::allocate(slot_alloc(), cap);
        capacity_ = cap;
    }

    HMM_CONSTEXPR_20 void deallocate_storage(const size_type cap) {
        deallocate_storage(ctrl_, slots_, cap);
    }

    HMM_CONSTEXPR_20 void deallocate_storage(Control* ctrl, Slot* slots,
                                             const size_type cap) {
        auto&& control_allocator = ctrl_alloc();
        CtrlTraits::deallocate(control_allocator, ctrl, cap);
        slot_traits::deallocate(slot_alloc(), slots, cap);
    }

    HMM_CONSTEXPR_20 void clear_elements() {
        for (std::size_t i = 0; i < capacity(); ++i) {
            if (ctrl_[i] >= 0) {
                policy_type::destroy(slot_alloc(), &slots_[i]);
            }
        }
    }

    HMM_CONSTEXPR_20 void clear_and_deallocate() {
        if (!slots_) {
            return;
        }
        clear_elements();
        deallocate_storage(capacity());
        ctrl_ = nullptr;
        slots_ = nullptr;
        capacity_ = 0;
        size_ = 0;
    }

    detail::CompressedTuple<Hash, Eq, slot_allocator, CtrlAllocator> members_;
    Control* ctrl_{nullptr};
    Slot* slots_{nullptr};
    size_type capacity_{0};
    size_type size_{0};
};

}  // namespace internal
}  // namespace hmm

#endif  // HMM_HMM_INTERNAL_RAW_HASH_SET_HPP
