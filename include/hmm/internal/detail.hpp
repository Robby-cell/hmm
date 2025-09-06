#ifndef HMM_INTERNAL_DETAIL_HPP
#define HMM_INTERNAL_DETAIL_HPP

#include <cstdint>
#include <type_traits>

#include "hmm/internal/macros.hpp"

namespace hmm {
namespace internal {
namespace detail {

template <class Ty>
struct type_identity {
    using type = Ty;
};

template <class Ty>
using remove_cvref_t =
    typename std::remove_cv<typename std::remove_reference<Ty>::type>::type;
template <class Ty>
struct remove_cvref : type_identity<remove_cvref_t<Ty>> {};

constexpr std::int8_t H1(const std::size_t hash) {
    return hash >> 57;  // Top 7 bits
}

constexpr std::size_t H2(const std::size_t hash, const std::size_t capacity) {
    return hash & (capacity - 1);
}

namespace slots {
constexpr std::int8_t kEmpty = -128;
constexpr std::int8_t kDeleted = -2;
}  // namespace slots

template <typename... Ts>
struct CompressedTuple : private Ts... {
    CompressedTuple() = default;
    CompressedTuple(const Ts&... args) : Ts(args)... {}

    template <int I, typename T>
    HMM_NODISCARD T& get() {
        return static_cast<T&>(*this);
    }

    template <int I, typename T>
    HMM_NODISCARD const T& get() const {
        return static_cast<const T&>(*this);
    }
};

namespace construction {
template <class...>
using _AlwaysTrue = std::true_type;

template <class T>
struct IsDirectInitializable {
    template <class U, class... Args>
    static auto is_it(Args&&... args)
        -> _AlwaysTrue<decltype(U(std::forward<Args>(args)...))>;
    template <class U, class... Args>
    static auto is_it(...) -> std::false_type;
};

template <class DirectInitializable>
struct Constructor;

template <>
struct Constructor<std::true_type> {
    template <class T, class... Args>
    static T construct(Args&&... args) {
        return T(std::forward<Args>(args)...);
    }
};

template <>
struct Constructor<std::false_type> {
    template <class T, class... Args>
    static T construct(Args&&... args) {
        return T{std::forward<Args>(args)...};
    }
};
}  // namespace construction

// std::is_aggregate does not exist in c++11
// This is a work-around to achieve what I need.
// If it cannot be constructed by direct initialization,
// aggregate initialization will be used.
template <class T, class... Args>
T construct(Args&&... args) {
    return construction::Constructor<
        decltype(construction::IsDirectInitializable<T>::template is_it<T>(
            std::forward<Args>(
                args)...))>::template construct<T>(std::forward<Args>(args)...);
}

}  // namespace detail
}  // namespace internal
}  // namespace hmm

#endif  // HMM_INTERNAL_DETAIL_HPP
