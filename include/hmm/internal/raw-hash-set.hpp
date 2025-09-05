#ifndef HMM_INTERNAL_RAW_HASH_SET_HPP
#define HMM_INTERNAL_RAW_HASH_SET_HPP

#include <cstdint>
#include <memory>

#include "hmm/internal/detail.hpp"
#include "hmm/internal/macros.hpp"

namespace hmm {
namespace detail {

template <class Policy, class Hash, class Eq, class Alloc>
class raw_hash_set {
   protected:
    using policy_type = Policy;
    using key_type = typename policy_type::key_type;
    using value_type = typename policy_type::value_type;
    using allocator_type = Alloc;
    using traits = std::allocator_traits<allocator_type>;
    using pointer = typename traits::pointer;

    using Control = std::int8_t;
    using Slot = value_type;

    template <typename Value>
    class HMM_NODISCARD iterator_impl {
        friend class raw_hash_set;

       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Value;
        using difference_type = std::ptrdiff_t;
        using pointer = Value*;
        using reference = Value&;

        constexpr iterator_impl() = default;

        template <typename OtherValue,
                  typename = std::enable_if_t<std::is_const_v<Value> &&
                                              !std::is_const_v<OtherValue>>>
        constexpr iterator_impl(const iterator_impl<OtherValue>& other)
            : ctrl_(other.ctrl_),
              slot_(other.slot_),
              end_ctrl_(other.end_ctrl_) {}

        HMM_NODISCARD constexpr reference operator*() const {
            return *slot_;
        }
        HMM_NODISCARD constexpr Slot* operator->() const {
            return slot_;
        }

        HMM_NODISCARD constexpr iterator_impl& operator++() {
            ++ctrl_;
            ++slot_;
            skip_empty_or_deleted();
            return *this;
        }

        HMM_NODISCARD constexpr iterator_impl operator++(int) {
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
        friend class raw_hash_set;

       public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Value;
        using difference_type = std::ptrdiff_t;
        using pointer = const Value*;
        using reference = const Value&;

        constexpr const_iterator_impl() = default;

        template <typename OtherValue,
                  typename = std::enable_if_t<std::is_const_v<Value> &&
                                              !std::is_const_v<OtherValue>>>
        constexpr const_iterator_impl(
            const const_iterator_impl<OtherValue>& other)
            : ctrl_(other.ctrl_),
              slot_(other.slot_),
              end_ctrl_(other.end_ctrl_) {}

        HMM_NODISCARD constexpr reference operator*() const {
            return *slot_;
        }
        HMM_NODISCARD constexpr const Slot* operator->() const {
            return slot_;
        }

        HMM_NODISCARD constexpr const_iterator_impl& operator++() {
            ++ctrl_;
            ++slot_;
            skip_empty_or_deleted();
            return *this;
        }

        HMM_NODISCARD constexpr const_iterator_impl operator++(int) {
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
        constexpr const_iterator_impl(Control* ctrl, Slot* slot,
                                      Control* end_ctrl)
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

    using iterator = iterator_impl<value_type>;
    using const_iterator = const_iterator_impl<const value_type>;

    struct HMM_NODISCARD FindInfo {
        std::size_t index;
        bool found;
    };

    HMM_CONSTEXPR_20 raw_hash_set() = default;

    HMM_CONSTEXPR_20 explicit raw_hash_set(const allocator_type& alloc)
        : members_(Hash{}, Eq{}, alloc) {}

    HMM_CONSTEXPR_20 ~raw_hash_set() {
        if (!slots_) {
            return;
        }
        clear_elements();
        deallocate_storage(capacity());
    }

    HMM_CONSTEXPR_20 raw_hash_set(raw_hash_set&& other) noexcept
        : members_(std::move(other.members_)),
          ctrl_(std::exchange(other.ctrl_, nullptr)),
          slots_(std::exchange(other.slots_, nullptr)),
          capacity_(std::exchange(other.capacity_, 0)),
          size_(std::exchange(other.size_, 0)) {}

    HMM_CONSTEXPR_20 raw_hash_set& operator=(raw_hash_set&& other) noexcept {
        if (this != &other) {
            clear_and_deallocate();
            members_ = std::move(other.members_);
            ctrl_ = std::exchange(other.ctrl_, nullptr);
            slots_ = std::exchange(other.slots_, nullptr);
            capacity_ = std::exchange(other.capacity_, 0);
            size_ = std::exchange(other.size_, 0);
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

    HMM_NODISCARD constexpr std::size_t size() const noexcept {
        return size_;
    }

    HMM_NODISCARD constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    HMM_NODISCARD constexpr std::size_t capacity() const noexcept {
        return capacity_;
    }

    HMM_CONSTEXPR_20 void clear() {
        if (!slots_) {
            return;
        }
        clear_elements();
        std::fill(ctrl_, ctrl_ + capacity(), slots::kEmpty);
        size_ = 0;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 FindInfo
    find_or_prepare_insert(const key_type& key) {
        if (capacity() == 0) {
            return {0, false};
        }
        const auto full_hash = hasher()(key);
        const auto h1 = H1(full_hash);
        auto pos = H2(full_hash, capacity());
        std::size_t first_deleted = -1;

        for (std::size_t i = 0; i < capacity(); ++i) {
            const std::size_t index = (pos + i) & (capacity() - 1);
            if (ctrl_[index] == slots::kEmpty) {
                return {(first_deleted != -1) ? first_deleted : index, false};
            }
            if (ctrl_[index] == slots::kDeleted) {
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
        const auto h1 = H1(full_hash);
        auto pos = H2(full_hash, capacity());

        for (std::size_t i = 0; i < capacity(); ++i) {
            const std::size_t index = (pos + i) & (capacity() - 1);
            if (ctrl_[index] == h1 &&
                equal()(key, policy_type::key(slots_[index]))) {
                return iterator(ctrl_ + index, slots_ + index,
                                ctrl_ + capacity());
            }
            if (ctrl_[index] == slots::kEmpty) {
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
        const auto h1 = H1(full_hash);
        auto pos = H2(full_hash, capacity());

        for (std::size_t i = 0; i < capacity(); ++i) {
            const std::size_t index = (pos + i) & (capacity() - 1);
            if (ctrl_[index] == h1 &&
                equal()(key, policy_type::key(slots_[index]))) {
                return const_iterator(ctrl_ + index, slots_ + index,
                                      ctrl_ + capacity());
            }
            if (ctrl_[index] == slots::kEmpty) {
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
        traits::destroy(alloc(), &slots_[index]);
        ctrl_[index] = slots::kDeleted;
        --size_;
        auto it =
            iterator(cit.ctrl_, const_cast<pointer>(cit.slot_), cit.end_ctrl_);
        it.skip_empty_or_deleted();
        return it;
    }

    HMM_CONSTEXPR_20 void erase_at(std::size_t index) {
        traits::destroy(alloc(), &slots_[index]);
        ctrl_[index] = slots::kDeleted;
        --size_;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 std::size_t erase_element(
        const key_type& key) {
        auto it = find(key);
        if (it == end()) {
            return 0;
        }
        erase(it);
        return 1;
    }

    HMM_CONSTEXPR_20 void rehash_and_grow() {
        auto old_capacity = capacity();
        auto old_ctrl = ctrl_;
        auto old_slots = slots_;

        const std::size_t new_cap = (old_capacity == 0) ? 16 : old_capacity * 2;
        allocate_storage(new_cap);
        std::fill(ctrl_, ctrl_ + capacity(), slots::kEmpty);

        size_ = 0;

        if (!old_slots) {
            return;
        }

        for (std::size_t i = 0; i < old_capacity; ++i) {
            if (old_ctrl[i] >= 0) {
                const auto& key = policy_type::key(old_slots[i]);
                auto info = find_or_prepare_insert(key);
                ctrl_[info.index] = H1(hasher()(key));
                traits::construct(alloc(), &slots_[info.index],
                                  std::move(old_slots[i]));
                traits::destroy(alloc(), &old_slots[i]);
                ++size_;
            }
        }

        deallocate_storage(old_ctrl, old_slots, old_capacity);
    }

    HMM_CONSTEXPR_20 void insert(const value_type& value) {
        emplace(value);
    }

    HMM_CONSTEXPR_20 void insert(value_type&& value) {
        emplace(std::move(value));
    }

    template <typename... Args>
    HMM_CONSTEXPR_20 std::pair<iterator, bool> emplace(Args&&... args) {
        if (needs_resize()) {
            rehash_and_grow();
        }
        value_type tmp(std::forward<Args>(args)...);
        auto info = find_or_prepare_insert(policy_type::key(tmp));
        if (info.found) {
            return {iterator(ctrl_ + info.index, slots_ + info.index,
                             ctrl_ + capacity()),
                    false};
        }
        const auto& key = policy_type::key(tmp);
        const auto full_hash = hasher()(key);
        ctrl_[info.index] = H1(full_hash);
        traits::construct(alloc(), &slots_[info.index], std::move(tmp));
        ++size_;
        return {iterator(ctrl_ + info.index, slots_ + info.index,
                         ctrl_ + capacity()),
                true};
    }

    HMM_NODISCARD constexpr Control*& ctrl_ptr() {
        return ctrl_;
    }
    HMM_NODISCARD constexpr Slot*& slots_ptr() {
        return slots_;
    }
    HMM_NODISCARD constexpr std::size_t& capacity_ref() {
        return capacity_;
    }
    HMM_NODISCARD constexpr std::size_t& size_ref() {
        return size_;
    }

    using CtrlAllocator = typename traits::template rebind_alloc<Control>;
    using CtrlTraits = std::allocator_traits<CtrlAllocator>;

    HMM_NODISCARD constexpr Hash& hasher() noexcept {
        return members_.template get<0, Hash>();
    }
    HMM_NODISCARD constexpr Eq& equal() noexcept {
        return members_.template get<1, Eq>();
    }
    HMM_NODISCARD constexpr allocator_type& alloc() noexcept {
        return members_.template get<2, allocator_type>();
    }
    HMM_NODISCARD constexpr CtrlAllocator& ctrl_alloc() noexcept {
        return members_.template get<3, CtrlAllocator>();
    }
    HMM_NODISCARD constexpr const Hash& hasher() const noexcept {
        return members_.template get<0, Hash>();
    }
    HMM_NODISCARD constexpr const Eq& equal() const noexcept {
        return members_.template get<1, Eq>();
    }
    HMM_NODISCARD constexpr const allocator_type& alloc() const noexcept {
        return members_.template get<2, allocator_type>();
    }
    HMM_NODISCARD constexpr const CtrlAllocator& ctrl_alloc() const noexcept {
        return members_.template get<3, CtrlAllocator>();
    }

    HMM_NODISCARD constexpr bool needs_resize() const {
        return capacity() == 0 || size() * 8 > capacity() * 7;
    }

    HMM_CONSTEXPR_20 void allocate_storage(std::size_t cap) {
        auto&& control_allocator = ctrl_alloc();
        ctrl_ = CtrlTraits::allocate(control_allocator, cap);
        slots_ = traits::allocate(alloc(), cap);
        capacity_ = cap;
    }

    HMM_CONSTEXPR_20 void deallocate_storage(std::size_t cap) {
        deallocate_storage(ctrl_, slots_, cap);
    }

    HMM_CONSTEXPR_20 void deallocate_storage(Control* ctrl, Slot* slots,
                                             std::size_t cap) {
        auto&& control_allocator = ctrl_alloc();
        CtrlTraits::deallocate(control_allocator, ctrl, cap);
        traits::deallocate(alloc(), slots, cap);
    }

    HMM_CONSTEXPR_20 void clear_elements() {
        for (std::size_t i = 0; i < capacity(); ++i) {
            if (ctrl_[i] >= 0) {
                traits::destroy(alloc(), &slots_[i]);
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

    CompressedTuple<Hash, Eq, allocator_type, CtrlAllocator> members_;
    Control* ctrl_{nullptr};
    Slot* slots_{nullptr};
    std::size_t capacity_{0};
    std::size_t size_{0};
};

}  // namespace detail
}  // namespace hmm

#endif  // HMM_INTERNAL_RAW_HASH_SET_HPP
