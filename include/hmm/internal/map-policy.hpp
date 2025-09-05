#ifndef HMM_INTERNAL_MAP_POLICY_HPP
#define HMM_INTERNAL_MAP_POLICY_HPP

#include <utility>

#include "hmm/internal/macros.hpp"

namespace hmm {
namespace detail {

template <typename K, typename V>
struct HMM_NODISCARD MapPolicy {
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K, V>;

    HMM_NODISCARD static constexpr const key_type& key(const value_type& pair) {
        return pair.first;
    }

    HMM_NODISCARD static constexpr const mapped_type& value(
        const value_type& pair) noexcept {
        return pair.second;
    }

    HMM_NODISCARD static constexpr mapped_type& value(
        value_type& pair) noexcept {
        return pair.second;
    }
};

template <typename K>
struct HMM_NODISCARD MapPolicy<K, void> {
    using key_type = K;
    using mapped_type = void;
    using value_type = K;

    static constexpr const key_type& key(const value_type& value) noexcept {
        return value;
    }

    static constexpr key_type& key(value_type& value) noexcept {
        return value;
    }

    static constexpr const key_type&& key(const value_type&& value) noexcept {
        return std::move(value);
    }

    static constexpr key_type&& key(value_type&& value) noexcept {
        return std::move(value);
    }
};

}  // namespace detail
}  // namespace hmm

#endif  // HMM_INTERNAL_MAP_POLICY_HPP
