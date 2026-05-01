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

#include <type_traits>
#include <utility>

#include "hmm/internal/detail.hpp"
#include "hmm/internal/macros.hpp"

#ifndef HMM_HMM_INTERNAL_COMPRESSED_TUPLE_HPP
#define HMM_HMM_INTERNAL_COMPRESSED_TUPLE_HPP

namespace hmm {
namespace internal {

// C++11 Polyfill for std::index_sequence
#if HMM_HAS_CXX_14
using std::index_sequence;
using std::index_sequence_for;
using std::make_index_sequence;
#else
template <size_t... Ints> struct index_sequence {
    using type = index_sequence;
    using value_type = size_t;

    static constexpr std::size_t size() noexcept {
        return sizeof...(Ints);
    }
};

template <std::size_t N, size_t... Next>
struct make_index_sequence_impl
    : public make_index_sequence_impl<N - 1, N - 1, Next...> {};

template <size_t... Next> struct make_index_sequence_impl<0, Next...> {
    using type = index_sequence<Next...>;
};

template <std::size_t N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;

template <class... Ts>
using index_sequence_for = make_index_sequence<sizeof...(Ts)>;
#endif

template <std::size_t Index, class T, class = void> struct TupleLeaf {
  public:
    HMM_CONSTEXPR_14 T& get() & noexcept {
        return value_;
    }

    constexpr const T& get() const& noexcept {
        return value_;
    }

    HMM_CONSTEXPR_14 T&& get() && noexcept {
        return static_cast<T&&>(value_);
    }

    constexpr const T&& get() const&& noexcept {
        return static_cast<const T&&>(value_);
    }

    HMM_CONSTEXPR_14 operator T&() & noexcept {
        return get();
    }

    constexpr operator const T&() const& noexcept {
        return get();
    }

    HMM_CONSTEXPR_14 operator T&&() && noexcept {
        return std::move(*this).get();
    }

    constexpr operator const T&&() const&& noexcept {
        return std::move(*this).get();
    }

    T value_;
};

template <std::size_t Index, class T>
struct TupleLeaf<Index, T,
                 typename std::enable_if<std::is_empty<T>::value, int>::type>
    : T {
  public:
    template <class Ty,
              typename std::enable_if<
                  !std::is_same<TupleLeaf, typename std::remove_cv<
                                               typename std::remove_reference<
                                                   T>::type>::type>::value &&
                      std::is_convertible<Ty&&, T>::value,
                  int>::type = 0>
    HMM_CONSTEXPR_20 inline TupleLeaf(Ty&& value)
        : T(static_cast<Ty&&>(value)) {}

    HMM_CONSTEXPR_14 T& get() & noexcept {
        return *this;
    }

    constexpr const T& get() const& noexcept {
        return *this;
    }

    HMM_CONSTEXPR_14 T&& get() && noexcept {
        return static_cast<T&&>(*this);
    }

    constexpr const T&& get() const&& noexcept {
        return static_cast<const T&&>(*this);
    }

    HMM_CONSTEXPR_14 operator T&() & noexcept {
        return get();
    }

    constexpr operator const T&() const& noexcept {
        return get();
    }

    HMM_CONSTEXPR_14 operator T&&() && noexcept {
        return std::move(*this).get();
    }

    constexpr operator const T&&() const&& noexcept {
        return std::move(*this).get();
    }
};

template <class, class... Ts> struct CompressedTupleImpl;

template <size_t... Is, class... Ts>
struct CompressedTupleImpl<index_sequence<Is...>, Ts...>
    : TupleLeaf<Is, Ts>... {
  public:
    template <size_t... Idxs, class... Args>
    HMM_CONSTEXPR_20 inline CompressedTupleImpl(
        index_sequence<Idxs...> /* sequence */, Args&&... args)
        : TupleLeaf<Idxs, typename detail::TypeAtIndex<
                              Idxs, detail::TypePack<Ts...>>::type>{
              static_cast<Args&&>(args)}... {}

  private:
    template <std::size_t I, class Self>
    static HMM_CONSTEXPR_14 auto GetImpl(Self&& self) noexcept
        -> decltype(detail::forward_like<Self>(
            static_cast<Self&&>(self)
                .TupleLeaf<I, typename detail::TypeAtIndex<
                                  I, detail::TypePack<Ts...>>::type>::get())) {
        return detail::forward_like<Self>(
            static_cast<Self&&>(self)
                .TupleLeaf<I, typename detail::TypeAtIndex<
                                  I, detail::TypePack<Ts...>>::type>::get());
    }

  public:
    template <std::size_t I>
    HMM_CONSTEXPR_14 auto get() & noexcept -> decltype(GetImpl<I>(*this)) {
        return GetImpl<I>(*this);
    }

    template <std::size_t I>
    constexpr auto get() const& noexcept -> decltype(GetImpl<I>(*this)) {
        return GetImpl<I>(*this);
    }

    template <std::size_t I>
    HMM_CONSTEXPR_14 auto get() && noexcept
        -> decltype(GetImpl<I>(std::move(*this))) {
        return GetImpl<I>(std::move(*this));
    }

    template <std::size_t I>
    constexpr auto get() const&& noexcept
        -> decltype(GetImpl<I>(std::move(*this))) {
        return GetImpl<I>(std::move(*this));
    }
};

template <class... Ts>
struct CompressedTuple : CompressedTupleImpl<index_sequence_for<Ts...>, Ts...> {
  private:
    using Base = CompressedTupleImpl<index_sequence_for<Ts...>, Ts...>;

  public:
    template <class... Args>
    HMM_CONSTEXPR_20 inline CompressedTuple(Args&&... args)
        : Base(index_sequence_for<Args...>{}, static_cast<Args&&>(args)...) {}

    using Base::get;
};

} // namespace internal
} // namespace hmm

#endif // HMM_HMM_INTERNAL_COMPRESSED_TUPLE_HPP
