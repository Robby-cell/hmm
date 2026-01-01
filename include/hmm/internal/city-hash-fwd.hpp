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

#ifndef HMM_HMM_INTERNAL_CITY_HASH_FWD_HPP
#define HMM_HMM_INTERNAL_CITY_HASH_FWD_HPP

#include <cstdint>
#include <utility>

#ifdef HMM_HASH_IMPL_INLINE
#define HMM_HASH_QUALIFIER inline
#else
#define HMM_HASH_QUALIFIER
#endif

namespace hmm {
namespace internal {

using uint64 = uint64_t;
using uint32 = uint32_t;
using uint128 = std::pair<uint64, uint64>;

constexpr inline uint64 Uint128Low64(const uint128& x) {
    return x.first;
}

constexpr inline uint64 Uint128High64(const uint128& x) {
    return x.second;
}

constexpr uint64 k0 = 0xc3a5c85c97cb3127ULL;
constexpr uint64 k1 = 0xb492b66fbe98f273ULL;
constexpr uint64 k2 = 0x9ae16a3b2f90404fULL;
constexpr uint64 kMul = 0x9ddfea08eb382d69ULL;

HMM_HASH_QUALIFIER uint64 CityHash64(const char* s, size_t len);

HMM_HASH_QUALIFIER uint64 CityHash64WithSeed(const char* s, size_t len,
                                             uint64 seed);

HMM_HASH_QUALIFIER uint128 CityHash128(const char* s, size_t len);

HMM_HASH_QUALIFIER uint128 CityHash128WithSeed(const char* s, size_t len,
                                               uint128 seed);

}  // namespace internal
}  // namespace hmm

#ifdef HMM_HASH_IMPL_INLINE
#include "hmm/internal/city-hash-impl.inl.hpp"
#endif

#endif
