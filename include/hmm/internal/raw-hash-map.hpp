#ifndef HMM_INTERNAL_RAW_HASH_MAP_HPP
#define HMM_INTERNAL_RAW_HASH_MAP_HPP

#include <functional>
#include <initializer_list>
#include <stdexcept>

#include "hmm/internal/macros.hpp"
#include "hmm/internal/raw-hash-set.hpp"

namespace hmm {
namespace internal {

template <class Policy, class Hash = std::hash<typename Policy::key_type>,
          class Eq = std::equal_to<typename Policy::key_type>,
          class Alloc = std::allocator<typename Policy::value_type>>
class raw_hash_map : public raw_hash_set<Policy, Hash, Eq, Alloc> {
    using Base = raw_hash_map::raw_hash_set;

   protected:
    using policy_type = typename Base::policy_type;
    using hasher_type = typename Base::hasher_type;
    using key_equal = typename Base::key_equal;
    using key_type = typename Base::key_type;
    using value_type = typename Base::value_type;
    using mapped_type = typename Policy::mapped_type;
    using size_type = typename Base::size_type;

    using slot_type = typename Base::slot_type;
    using slot_allocator = typename Base::slot_allocator;
    using slot_traits = typename Base::slot_traits;
    using pointer = typename Base::pointer;
    using allocator_type = typename Base::allocator_type;

    using typename Base::Control;
    using typename Base::Slot;

    using typename Base::const_iterator;
    using typename Base::iterator;

    HMM_CONSTEXPR_20 raw_hash_map() = default;

    HMM_CONSTEXPR_20 raw_hash_map(
        std::initializer_list<value_type> initial,
        const allocator_type& alloc = allocator_type())
        : Base(initial, alloc) {}

    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 raw_hash_map(
        Iter begin, Sentinel end,
        const allocator_type& alloc = allocator_type())
        : Base(begin, end, alloc) {}

    HMM_CONSTEXPR_20 explicit raw_hash_map(const allocator_type& alloc)
        : Base(alloc) {}

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

    HMM_CONSTEXPR_20 iterator erase(const_iterator pos) {
        return Base::erase(pos);
    }

    HMM_CONSTEXPR_20 std::size_t erase(const key_type& key) {
        return Base::erase_element(key);
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& operator[](
        const key_type& key) {
        if (Base::needs_resize()) {
            Base::rehash_and_grow();
        }
        auto info = Base::find_or_prepare_insert(key);
        if (info.found) {
            return policy_type::value(Base::slots_ptr()[info.index]);
        }

        const auto full_hash = Base::hasher()(key);
        Base::ctrl_ptr()[info.index] = detail::H1(full_hash);

        policy_type::construct(Base::slot_alloc(),
                               &Base::slots_ptr()[info.index],
                               std::piecewise_construct,
                               std::forward_as_tuple(key), std::tuple<>());
        ++Base::size_ref();
        return policy_type::value(Base::slots_ptr()[info.index]);
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& at(const key_type& key) {
        auto it = find(key);
        auto my_end = end();
        if (it == my_end) {
            ThrowOutOfRange("raw_hash_map<>::at()");
        }
        return it->second;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 const mapped_type& at(
        const key_type& key) const {
        auto it = find(key);
        auto my_end = end();
        if (it == my_end) {
            ThrowOutOfRange("raw_hash_map<>::at()const");
        }
        return it->second;
    }

    template <class... Args>
    [[noreturn]] static void ThrowOutOfRange(Args&&... args) {
        throw std::out_of_range(std::forward<Args>(args)...);
    }
};

}  // namespace internal
}  // namespace hmm

#endif  // HMM_INTERNAL_RAW_HASH_MAP_HPP
