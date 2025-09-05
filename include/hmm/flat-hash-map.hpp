#ifndef HMM_HASH_MAP_HPP
#define HMM_HASH_MAP_HPP

#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>

#include "hmm/hasher.hpp"
#include "hmm/internal/macros.hpp"
#include "hmm/internal/raw-hash-map.hpp"

#if HMM_HAS_CXX_17
#include <memory_resource>
#endif

namespace hmm {

template <typename K, typename V>
struct HMM_NODISCARD MapPolicy;

template <class Key, class Value, class Hash = Hasher<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Alloc =
              std::allocator<typename MapPolicy<Key, Value>::value_type>>
class flat_hash_map : public detail::raw_hash_map<MapPolicy<Key, Value>, Hash,
                                                  KeyEqual, Alloc> {
    using Base = flat_hash_map::raw_hash_map;

   public:
    using policy_type = typename Base::policy_type;
    using hasher_type = typename Base::hasher_type;
    using key_equal = typename Base::key_equal;
    using key_type = typename Base::key_type;
    using value_type = typename Base::value_type;
    using mapped_type = typename Base::mapped_type;
    using size_type = typename Base::size_type;

    using slot_type = typename Base::slot_type;
    using slot_allocator = typename Base::slot_allocator;
    using slot_traits = typename Base::slot_traits;
    using pointer = typename Base::pointer;
    using allocator_type = typename Base::allocator_type;

    using typename Base::const_iterator;
    using typename Base::iterator;

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

template <typename K, typename V>
struct HMM_NODISCARD MapPolicy {
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K, V>;
    using slot_type = std::pair<K, V>;

    HMM_NODISCARD static constexpr const key_type& key(
        const slot_type& pair) noexcept {
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
template <class Key, class Value, class Hash = Hasher<Key>,
          class Eq = std::equal_to<Key>>
using flat_hash_map =
    ::hmm::flat_hash_map<Key, Value, Hash, Eq,
                         std::pmr::polymorphic_allocator<
                             typename MapPolicy<Key, Value>::value_type>>;
}
#endif

}  // namespace hmm

#endif  // HMM_HASH_MAP_HPP
