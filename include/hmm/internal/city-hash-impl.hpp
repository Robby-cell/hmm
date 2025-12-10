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

#ifndef HMM_HMM_INTERNAL_CITY_HASH_IMPL_HPP
#define HMM_HMM_INTERNAL_CITY_HASH_IMPL_HPP

#include <cstdint>
#include <cstring>
#include <utility>

namespace hmm {
namespace internal {

using uint64 = uint64_t;
using uint32 = uint32_t;
using uint128 = std::pair<uint64, uint64>;

static const uint64 k0 = 0xc3a5c85c97cb3127ULL;
static const uint64 k1 = 0xb492b66fbe98f273ULL;
static const uint64 k2 = 0x9ae16a3b2f90404fULL;
static const uint64 kMul = 0x9ddfea08eb382d69ULL;

static inline uint64 Rotate(uint64 val, int shift) {
    return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
}

static inline uint64 ShiftMix(uint64 val) {
    return val ^ (val >> 47);
}

static inline uint64 Hash128to64(uint128 x) {
    uint64 a = (x.first ^ x.second) * kMul;
    a ^= (a >> 47);
    uint64 b = (x.second ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}

static inline uint64 Fetch64(const char* p) {
    uint64 result;
    std::memcpy(&result, p, sizeof(result));
    return result;
}

static inline uint32 Fetch32(const char* p) {
    uint32 result;
    std::memcpy(&result, p, sizeof(result));
    return result;
}

static inline uint64 HashLen16(uint64 u, uint64 v) {
    return Hash128to64({u, v});
}

static inline uint64 HashLen16(uint64 u, uint64 v, uint64 mul) {
    uint64 a = (u ^ v) * mul;
    a ^= (a >> 47);
    uint64 b = (v ^ a) * mul;
    b ^= (b >> 47);
    b *= mul;
    return b;
}

static uint64 HashLen0to16(const char* s, size_t len) {
    if (len >= 8) {
        uint64 mul = k2 + len * 2;
        uint64 a = Fetch64(s) + k2;
        uint64 b = Fetch64(s + len - 8);
        uint64 c = Rotate(b, 37) * mul + a;
        uint64 d = (Rotate(a, 25) + b) * mul;
        return HashLen16(c, d, mul);
    }
    if (len >= 4) {
        uint64 mul = k2 + len * 2;
        uint64 a = Fetch32(s);
        return HashLen16(len + (a << 3), Fetch32(s + len - 4), mul);
    }
    if (len > 0) {
        uint8_t a = static_cast<uint8_t>(s[0]);
        uint8_t b = static_cast<uint8_t>(s[len >> 1]);
        uint8_t c = static_cast<uint8_t>(s[len - 1]);
        uint32 y = static_cast<uint32>(a) + (static_cast<uint32>(b) << 8);
        uint32 z = static_cast<uint32>(len) + (static_cast<uint32>(c) << 2);
        return ShiftMix(y * k2 ^ z * k0) * k2;
    }
    return k2;
}

static uint64 HashLen17to32(const char* s, size_t len) {
    uint64 mul = k2 + len * 2;
    uint64 a = Fetch64(s) * k1;
    uint64 b = Fetch64(s + 8);
    uint64 c = Fetch64(s + len - 8) * mul;
    uint64 d = Fetch64(s + len - 16) * k2;
    return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d,
                     a + Rotate(b + k2, 18) + c, mul);
}

static uint64 HashLen33to64(const char* s, size_t len) {
    uint64 z = Fetch64(s + 24);
    uint64 a = Fetch64(s) + (len + Fetch64(s + len - 16)) * k0;
    uint64 b = Rotate(a + z, 52);
    uint64 c = Rotate(a, 37);
    a += Fetch64(s + 8);
    c += Rotate(a, 7);
    a += Fetch64(s + 16);
    uint64 vf = a + z;
    uint64 vs = b + Rotate(a, 31) + c;
    a = Fetch64(s + 16) + Fetch64(s + len - 32);
    z = Fetch64(s + len - 8);
    b = Rotate(a + z, 52);
    c = Rotate(a, 37);
    a += Fetch64(s + len - 24);
    c += Rotate(a, 7);
    a += Fetch64(s + len - 16);
    uint64 wf = a + z;
    uint64 ws = b + Rotate(a, 31) + c;
    uint64 r = ShiftMix((vf + ws) * k2 + (wf + vs) * k0);
    return ShiftMix(r * k0 + vs) * k2;
}

static uint64 WeakHashLen32WithSeeds(uint64 w, uint64 x, uint64 y, uint64 z,
                                     uint64 a, uint64 b) {
    a += w;
    b = Rotate(b + a + z, 21);
    uint64 c = a;
    a += x;
    a += y;
    b += Rotate(a, 44);
    return HashLen16(c, a + z, b);
}

static uint64 WeakHashLen32WithSeeds(const char* s, uint64 a, uint64 b) {
    return WeakHashLen32WithSeeds(Fetch64(s), Fetch64(s + 8), Fetch64(s + 16),
                                  Fetch64(s + 24), a, b);
}

static uint64 CityHash64(const char* s, size_t len) {
    if (len <= 16) {
        return HashLen0to16(s, len);
    }
    if (len <= 32) {
        return HashLen17to32(s, len);
    }
    if (len <= 64) {
        return HashLen33to64(s, len);
    }

    uint64 x = Fetch64(s);
    uint64 y = Fetch64(s + len - 16) ^ k1;
    uint64 z = Fetch64(s + len - 56) ^ k0;
    uint64 v = WeakHashLen32WithSeeds(s + len - 64, len, y);
    uint64 w = WeakHashLen32WithSeeds(s + len - 32, len * k1, k0);
    z += ShiftMix(v) * k1;
    x = Rotate(z + x, 39) * k1;
    y = Rotate(y, 33) * k1;

    len = (len - 1) & ~static_cast<size_t>(63);
    do {
        x = Rotate(x + y + v + Fetch64(s + 16), 37) * k1;
        y = Rotate(y + v + Fetch64(s + 48), 42) * k1;
        x ^= w;
        y ^= v;
        z = Rotate(z ^ w, 33);
        v = WeakHashLen32WithSeeds(s, v * k1, x + w);
        w = WeakHashLen32WithSeeds(s + 32, z + w, y);
        std::swap(z, x);
        s += 64;
        len -= 64;
    } while (len != 0);

    return HashLen16(HashLen16(v, w) + ShiftMix(y) * k1 + z,
                     HashLen16(x, z) + w);
}

static uint64 CityHash64WithSeed(const char* s, size_t len, uint64 seed) {
    return CityHash64(s, len) * k1 + Rotate(seed ^ k0, 41);
}

}  // namespace internal
}  // namespace hmm

#endif  // HMM_HMM_INTERNAL_CITY_HASH_IMPL_HPP
