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

#ifndef HMM_HMM_HASHER_HPP
#define HMM_HMM_HASHER_HPP

#include "hmm/internal/city-hash-mixers.hpp"
#include "hmm/internal/macros.hpp"

namespace hmm {

template <typename T>
struct CityHash {
    HMM_STATIC_CALL size_t operator()(const T& value) HMM_STATIC_CALL_CONST {
        return internal::CityHashState::combine(
                   internal::CityHashState::Create(), value)
            .finalize();
    }
};

// Void specialization (often used in transparent comparators/hashers)
template <>
struct CityHash<void> {
    template <typename T>
    HMM_STATIC_CALL size_t operator()(const T& value) HMM_STATIC_CALL_CONST {
        return CityHash<T>{}(value);
    }
};

}  // namespace hmm

// ============================================================================
// How to create a custom hashable type:
// ============================================================================
// struct CustomType {
//     int x;
//     float y;
//     // Friend function found via ADL.
//     // No "hmm::" prefix needed here.
//     template <typename H>
//     friend H HashValue(H h, const CustomType& self) {
//         return H::combine(std::move(h), self.x, self.y);
//     }
// };

#endif  // HMM_HMM_HASHER_HPP
