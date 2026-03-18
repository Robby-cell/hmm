// Copyright 2025 Robert Williamson
//
// Licensed under the MIT License;
// You may not used this file except in compliance with the License.
// You may obtain a copy of the License at
//
//       https://opensource.org/license/mit
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef HMM_HMM_INTERNAL_DETAIL_HPP
#define HMM_HMM_INTERNAL_DETAIL_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

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

constexpr size_t H1(const size_t hash) {
    return hash;
}

constexpr std::int8_t H2(const size_t hash) {
    return hash >> (sizeof(size_t) * 8 - 7);
}

HMM_CONSTEXPR_14 inline size_t IndexWithoutProbing(
    const size_t h1, const size_t capacity) noexcept {
    return h1 & (capacity - 1);
}

namespace slots {
constexpr std::int8_t kEmpty = -128;
constexpr std::int8_t kDeleted = -2;
}  // namespace slots

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

template <class T, class U>
T exchange(T& self, U&& other) {
    T old = std::move(self);
    self = std::forward<U>(other);
    return old;
}

template <class T, class U>
struct ForwardLike {
    using type = U&&;
};

template <class T, class U>
struct ForwardLike<T&&, U> {
    using type = U&&;
};

template <class T, class U>
struct ForwardLike<T&, U> {
    using type = U&;
};

template <class T, class U>
struct ForwardLike<const T&&, U> {
    using type = const U&&;
};

template <class T, class U>
struct ForwardLike<const T&, U> {
    using type = const U&;
};

template <class Like, class Type>
HMM_NODISCARD constexpr typename detail::ForwardLike<Like, Type>::type
forward_like(Type&& value) noexcept {
    return static_cast<typename detail::ForwardLike<Like, Type>::type>(value);
}

template <class... Ts>
struct TypePack;

template <size_t I, class T, class... Ts>
struct TypeAtIndexImpl {
    using type = typename TypeAtIndexImpl<I - 1, Ts...>::type;
};

template <class T, class... Ts>
struct TypeAtIndexImpl<0, T, Ts...> {
    using type = T;
};

template <size_t I, class TP>
struct TypeAtIndex;

template <size_t I, class... Ts>
struct TypeAtIndex<I, TypePack<Ts...>> {
    static_assert(I < sizeof...(Ts), "Index must be within the list of types");

    using type = typename TypeAtIndexImpl<I, Ts...>::type;
};

/// @brief Helper trait to extract the I-th type from a parameter pack,
/// falling back to a Default type if the pack is too short.
template <size_t Index, class Default, class... Args>
struct TypeAtIndexOrDefault;

// Base case: We reached index 0 and a type is available.
template <class Default, class First, class... Rest>
struct TypeAtIndexOrDefault<0, Default, First, Rest...> {
    using type = First;
};

// Recursive case: Decrement index and discard the first type.
template <size_t Index, class Default, class First, class... Rest>
struct TypeAtIndexOrDefault<Index, Default, First, Rest...> {
    using type =
        typename TypeAtIndexOrDefault<Index - 1, Default, Rest...>::type;
};

// Fallback case: The pack is empty, use the Default type.
template <size_t Index, class Default>
struct TypeAtIndexOrDefault<Index, Default> {
    using type = Default;
};

}  // namespace detail
}  // namespace internal
}  // namespace hmm

#endif  // HMM_HMM_INTERNAL_DETAIL_HPP
