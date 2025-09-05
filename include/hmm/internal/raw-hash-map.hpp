#ifndef HMM_INTERNAL_RAW_HASH_MAP_HPP
#define HMM_INTERNAL_RAW_HASH_MAP_HPP

#include <functional>
#include <initializer_list>
#include <stdexcept>

#include "hmm/internal/macros.hpp"
#include "hmm/internal/raw-hash-set.hpp"
#include "macros.hpp"

namespace hmm {
namespace detail {

template <class Policy, class Hash = std::hash<typename Policy::key_type>,
          class Eq = std::equal_to<typename Policy::key_type>,
          class Alloc = std::allocator<typename Policy::value_type>>
class raw_hash_map : public raw_hash_set<Policy, Hash, Eq, Alloc> {
    using Base = raw_hash_set<Policy, Hash, Eq, Alloc>;

   protected:
    using typename Base::key_type;
    using typename Base::value_type;
    using mapped_type = typename Policy::mapped_type;
    using typename Base::allocator_type;
    using typename Base::pointer;
    using typename Base::policy_type;

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
            return Base::slots_ptr()[info.index].second;
        }

        const auto full_hash = Base::hasher()(key);
        Base::ctrl_ptr()[info.index] = H1(full_hash);

        using traits = typename Base::traits;
        traits::construct(Base::alloc(), &Base::slots_ptr()[info.index],
                          std::piecewise_construct, std::forward_as_tuple(key),
                          std::tuple<>());
        ++Base::size_ref();
        return Base::slots_ptr()[info.index].second;
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

}  // namespace detail
}  // namespace hmm

#endif  // HMM_INTERNAL_RAW_HASH_MAP_HPP
