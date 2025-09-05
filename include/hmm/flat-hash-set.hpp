#ifndef HMM_FLAT_HASH_SET_HPP
#define HMM_FLAT_HASH_SET_HPP

#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>

#include "hmm/hasher.hpp"
#include "hmm/internal/macros.hpp"
#include "hmm/internal/raw-hash-set.hpp"

namespace hmm {

template <typename T>
struct HMM_NODISCARD SetPolicy;

template <class Contained, class Hash = Hasher<Contained>,
          class Eq = std::equal_to<Contained>,
          class Alloc =
              typename std::allocator<typename SetPolicy<Contained>::key_type>>
class flat_hash_set
    : detail::raw_hash_set<SetPolicy<Contained>, Hash, Eq, Alloc> {
    using Base = flat_hash_set::raw_hash_set;

   public:
    using policy_type = typename Base::policy_type;
    using hasher_type = typename Base::hasher_type;
    using key_equal = typename Base::key_equal;
    using key_type = typename Base::key_type;
    using value_type = typename Base::value_type;
    using size_type = typename Base::size_type;

    using slot_type = typename Base::slot_type;
    using slot_allocator = typename Base::slot_allocator;
    using slot_traits = typename Base::slot_traits;
    using pointer = typename Base::pointer;
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

    HMM_CONSTEXPR_20 explicit flat_hash_set(const allocator_type& alloc)
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

template <typename T>
struct HMM_NODISCARD SetPolicy {
    using key_type = T;
    using mapped_type = void;
    using value_type = T;
    using slot_type = T;

    HMM_NODISCARD static constexpr const key_type& key(
        const slot_type& slot) noexcept {
        return slot;
    }

    HMM_NODISCARD static constexpr value_type& value_from_slot(
        slot_type& slot) noexcept {
        return slot;
    }

    HMM_NODISCARD static constexpr const value_type& value_from_slot(
        const slot_type& slot) noexcept {
        return slot;
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

}  // namespace hmm

#endif  // HMM_FLAT_HASH_SET_HPP
