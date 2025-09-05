#ifndef HMM_HASHER_HPP
#define HMM_HASHER_HPP

#include <functional>
#include <utility>

#include "hmm/internal/detail.hpp"
#include "hmm/internal/macros.hpp"

namespace hmm {

template <class Ty = void>
struct Hasher {
    HMM_NODISCARD HMM_CONSTEXPR_20 HMM_STATIC_CALL std::size_t operator()(
        const Ty& value) HMM_STATIC_CALL_CONST {
        return std::hash<Ty>{}(value);
    }
};

template <>
struct Hasher<void> {
    template <class Ty>
    HMM_NODISCARD HMM_CONSTEXPR_20 HMM_STATIC_CALL std::size_t operator()(
        Ty&& value) HMM_STATIC_CALL_CONST {
        return Hasher<detail::remove_cvref_t<Ty>>{}(std::forward<Ty>(value));
    }
};

}  // namespace hmm

#endif  // HMM_HASHER_HPP
