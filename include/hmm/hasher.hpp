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
        return Hasher<internal::detail::remove_cvref_t<Ty>>{}(
            std::forward<Ty>(value));
    }
};

}  // namespace hmm

#endif  // HMM_HASHER_HPP
