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

#ifndef HMM_HMM_INTERNAL_BIT_MASK_HPP
#define HMM_HMM_INTERNAL_BIT_MASK_HPP

#include <cstdint>
#include <cstring>

#include "hmm/internal/macros.hpp"

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

// Detect SIMD Platform
#if defined(__SSE2__) || \
    (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
#define HMM_SSE2 1
#include <emmintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define HMM_NEON 1
#include <arm_neon.h>
#endif

namespace hmm {
namespace internal {

// Portable Count Trailing Zeros
inline uint32_t CountTrailingZeros(uint32_t n) {
#if defined(_MSC_VER)
    unsigned long index;
    if (_BitScanForward(&index, n)) {
        return index;
    }
    return 32;
#else
    return n == 0 ? 32 : __builtin_ctz(n);
#endif
}

// Wrapper for the result of a SIMD comparison
class BitMask {
   public:
    constexpr explicit BitMask(uint32_t mask) : mask_(mask) {}

    // Iterator-like interface for "for (auto i : mask)" loops
    constexpr BitMask begin() const {
        return *this;
    }

    uint32_t first_index() const {
        return CountTrailingZeros(mask_);
    }

    // Clear the lowest set bit
    HMM_CONSTEXPR_14 BitMask& operator++() {
        mask_ &= (mask_ - 1);
        return *this;
    }

    constexpr explicit operator bool() const {
        return mask_ != 0;
    }

   private:
    uint32_t mask_;
};

// Abstraction for 16 bytes of control data
struct Group {
#if defined(HMM_SSE2)
    __m128i data;
#elif defined(HMM_NEON)
    uint8x16_t data;
#else
    uint64_t data[2];  // Fallback
#endif

    static constexpr size_t kWidth = 16;

    // Load 16 bytes from memory (potentially unaligned)
    static Group Load(const int8_t* ptr) {
        Group g;
#if defined(HMM_SSE2)
        g.data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
#elif defined(HMM_NEON)
        g.data = vld1q_u8(reinterpret_cast<const uint8_t*>(ptr));
#else
        std::memcpy(&g.data, ptr, 16);
#endif
        return g;
    }

    // Returns a mask where 1 bits indicate the byte equals h2
    BitMask Match(int8_t h2) const {
#if defined(HMM_SSE2)
        auto match = _mm_cmpeq_epi8(data, _mm_set1_epi8(h2));
        return BitMask(_mm_movemask_epi8(match));
#elif defined(HMM_NEON)
        // vshrn to narrow to 64bit then vget_lane simplified scalar fallback
        // for safety
        uint32_t mask = 0;
        uint8_t buf[16];
        vst1q_u8(buf, data);
        for (int i = 0; i < 16; ++i) {
            if (buf[i] == h2) {
                mask |= (1 << i);
            }
        }
        return BitMask(mask);
#else
        uint32_t mask = 0;
        const int8_t* bytes = reinterpret_cast<const int8_t*>(&data);
        for (size_t i = 0; i < 16; ++i) {
            if (bytes[i] == h2) {
                mask |= (1 << i);
            }
        }
        return BitMask(mask);
#endif
    }

    // Returns a mask where 1 bits indicate the byte is Empty (-128)
    BitMask MatchEmpty() const {
#if defined(HMM_SSE2)
        auto match =
            _mm_cmpeq_epi8(data, _mm_set1_epi8(static_cast<int8_t>(-128)));
        return BitMask(_mm_movemask_epi8(match));
#else
        uint32_t mask = 0;
        const int8_t* bytes = reinterpret_cast<const int8_t*>(&data);
        for (size_t i = 0; i < 16; ++i) {
            if (bytes[i] == static_cast<int8_t>(-128)) {
                mask |= (1 << i);
            }
        }
        return BitMask(mask);
#endif
    }
};

}  // namespace internal
}  // namespace hmm

#endif
