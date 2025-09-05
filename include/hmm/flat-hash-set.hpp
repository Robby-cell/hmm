#ifndef HMM_FLAT_HASH_SET_HPP
#define HMM_FLAT_HASH_SET_HPP

#include <functional>
#include <initializer_list>
#include <memory>

#include "hmm/hasher.hpp"
#include "hmm/internal/macros.hpp"
#include "hmm/internal/map-policy.hpp"
#include "hmm/internal/raw-hash-set.hpp"
#include "internal/macros.hpp"

namespace hmm {

template <class Contained, class Hash = Hasher<Contained>,
          class Eq = std::equal_to<Contained>,
          class Alloc = typename std::allocator<
              typename detail::MapPolicy<Contained, void>::key_type>>
class flat_hash_set : detail::raw_hash_set<detail::MapPolicy<Contained, void>,
                                           Hash, Eq, Alloc> {
    using Base = detail::raw_hash_set<detail::MapPolicy<Contained, void>, Hash,
                                      Eq, Alloc>;

   public:
    using key_type = Contained;
    using value_type = typename detail::MapPolicy<Contained, void>::value_type;
    using size_type = std::size_t;
    using hasher = Hash;
    using key_equal = Eq;
    using allocator_type = typename Base::allocator_type;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

    HMM_CONSTEXPR_20 flat_hash_set() = default;

    HMM_CONSTEXPR_20 flat_hash_set(
        std::initializer_list<value_type> initial,
        const allocator_type& alloc = allocator_type())
        : Base(initial, alloc) {}

    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 flat_hash_set(
        Iter begin, Sentinel end,
        const allocator_type& alloc = allocator_type())
        : Base(begin, end, alloc) {}

    HMM_CONSTEXPR_20 explicit flat_hash_set(
        const allocator_type& alloc = allocator_type())
        : Base(alloc) {}

    using Base::begin;
    using Base::capacity;
    using Base::cbegin;
    using Base::cend;
    using Base::clear;
    using Base::contains;
    using Base::emplace;
    using Base::empty;
    using Base::end;
    using Base::erase;
    using Base::find;
    using Base::insert;
    using Base::reserve;
    using Base::size;
};

}  // namespace hmm

#endif  // HMM_FLAT_HASH_SET_HPP
