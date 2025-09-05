#ifndef HMM_HASH_MAP_HPP
#define HMM_HASH_MAP_HPP

#include <functional>
#include <initializer_list>
#include <memory>

#include "hmm/hasher.hpp"
#include "hmm/internal/macros.hpp"
#include "hmm/internal/map-policy.hpp"
#include "hmm/internal/raw-hash-map.hpp"
#include "internal/macros.hpp"

namespace hmm {

template <class Key, class Value, class Hash = Hasher<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Alloc = std::allocator<
              typename detail::MapPolicy<Key, Value>::value_type>>
class flat_hash_map : public detail::raw_hash_map<detail::MapPolicy<Key, Value>,
                                                  Hash, KeyEqual, Alloc> {
    using Base = detail::raw_hash_map<detail::MapPolicy<Key, Value>, Hash,
                                      KeyEqual, Alloc>;

   public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using size_type = std::size_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = typename Base::allocator_type;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;

    HMM_CONSTEXPR_20 flat_hash_map() = default;

    HMM_CONSTEXPR_20 flat_hash_map(
        std::initializer_list<value_type> initial,
        const allocator_type& alloc = allocator_type())
        : Base(initial, alloc) {}

    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 flat_hash_map(
        Iter begin, Sentinel end,
        const allocator_type& alloc = allocator_type())
        : Base(begin, end, alloc) {}

    HMM_CONSTEXPR_20
    explicit flat_hash_map(const allocator_type& alloc) : Base(alloc) {}

    using Base::begin;
    using Base::capacity;
    using Base::cbegin;
    using Base::cend;
    using Base::clear;
    using Base::empty;
    using Base::end;
    using Base::erase;
    using Base::find;
    using Base::insert;
    using Base::size;
    using Base::operator[];
    using Base::at;
    using Base::contains;
    using Base::reserve;
};

}  // namespace hmm

#endif  // HMM_HASH_MAP_HPP
