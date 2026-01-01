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

#include "hmm/internal/city-hash-fwd.hpp"

namespace hmm {
namespace internal {

// Implementation from: https://github.com/google/cityhash

#if !defined(LIKELY)
#if HAVE_BUILTIN_EXPECT
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#else
#define LIKELY(x) (x)
#endif
#endif

namespace {

inline uint64 Rotate(uint64 val, int shift) {
    return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
}

inline uint64 ShiftMix(uint64 val) {
    return val ^ (val >> 47);
}

inline uint64 Hash128to64(uint128 x) {
    uint64 a = (x.first ^ x.second) * kMul;
    a ^= (a >> 47);
    uint64 b = (x.second ^ a) * kMul;
    b ^= (b >> 47);
    b *= kMul;
    return b;
}

inline uint64 Fetch64(const char* p) {
    uint64 result;
    std::memcpy(&result, p, sizeof(result));
    return result;
}

inline uint32 Fetch32(const char* p) {
    uint32 result;
    std::memcpy(&result, p, sizeof(result));
    return result;
}

inline uint64 HashLen16(uint64 u, uint64 v) {
    return Hash128to64({u, v});
}

inline uint64 HashLen16(uint64 u, uint64 v, uint64 mul) {
    uint64 a = (u ^ v) * mul;
    a ^= (a >> 47);
    uint64 b = (v ^ a) * mul;
    b ^= (b >> 47);
    b *= mul;
    return b;
}

uint64 HashLen0to16(const char* s, size_t len) {
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

uint64 HashLen17to32(const char* s, size_t len) {
    uint64 mul = k2 + len * 2;
    uint64 a = Fetch64(s) * k1;
    uint64 b = Fetch64(s + 8);
    uint64 c = Fetch64(s + len - 8) * mul;
    uint64 d = Fetch64(s + len - 16) * k2;
    return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d,
                     a + Rotate(b + k2, 18) + c, mul);
}

uint64 HashLen33to64(const char* s, size_t len) {
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

std::pair<uint64, uint64> WeakHashLen32WithSeeds(uint64 w, uint64 x, uint64 y,
                                                 uint64 z, uint64 a, uint64 b) {
    a += w;
    b = Rotate(b + a + z, 21);
    uint64 c = a;
    a += x;
    a += y;
    b += Rotate(a, 44);
    return std::make_pair(a + z, b + c);
}

std::pair<uint64, uint64> WeakHashLen32WithSeeds(const char* s, uint64 a,
                                                 uint64 b) {
    return WeakHashLen32WithSeeds(Fetch64(s), Fetch64(s + 8), Fetch64(s + 16),
                                  Fetch64(s + 24), a, b);
}

uint64 CityHash64WithSeeds(const char* s, size_t len, uint64 seed0,
                           uint64 seed1) {
    return HashLen16(CityHash64(s, len) - seed0, seed1);
}

uint128 CityMurmur(const char* s, size_t len, uint128 seed) {
    uint64 a = Uint128Low64(seed);
    uint64 b = Uint128High64(seed);
    uint64 c = 0;
    uint64 d = 0;
    if (len <= 16) {
        a = ShiftMix(a * k1) * k1;
        c = b * k1 + HashLen0to16(s, len);
        d = ShiftMix(a + (len >= 8 ? Fetch64(s) : c));
    } else {
        c = HashLen16(Fetch64(s + len - 8) + k1, a);
        d = HashLen16(b + len, c + Fetch64(s + len - 16));
        a += d;
        do {
            a ^= ShiftMix(Fetch64(s) * k1) * k1;
            a *= k1;
            b ^= a;
            c ^= ShiftMix(Fetch64(s + 8) * k1) * k1;
            c *= k1;
            d ^= c;
            s += 16;
            len -= 16;
        } while (len > 16);
    }
    a = HashLen16(a, c);
    b = HashLen16(d, b);
    return uint128(a ^ b, HashLen16(b, a));
}

}  // namespace

HMM_HASH_QUALIFIER uint64 CityHash64(const char* s, size_t len) {
    if (len <= 16) {
        return HashLen0to16(s, len);
    }
    if (len <= 32) {
        return HashLen17to32(s, len);
    }
    if (len <= 64) {
        return HashLen33to64(s, len);
    }

    uint64 x = Fetch64(s + len - 40);
    uint64 y = Fetch64(s + len - 16) + Fetch64(s + len - 56);
    uint64 z = HashLen16(Fetch64(s + len - 48) + len, Fetch64(s + len - 24));
    std::pair<uint64, uint64> v = WeakHashLen32WithSeeds(s + len - 64, len, z);
    std::pair<uint64, uint64> w =
        WeakHashLen32WithSeeds(s + len - 32, y + k1, x);
    x = x * k1 + Fetch64(s);

    len = (len - 1) & ~static_cast<size_t>(63);
    do {
        x = Rotate(x + y + v.first + Fetch64(s + 8), 37) * k1;
        y = Rotate(y + v.second + Fetch64(s + 48), 42) * k1;
        x ^= w.second;
        y += v.first + Fetch64(s + 40);
        z = Rotate(z + w.first, 33) * k1;
        v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
        w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
        std::swap(z, x);
        s += 64;
        len -= 64;
    } while (len != 0);
    return HashLen16(HashLen16(v.first, w.first) + ShiftMix(y) * k1 + z,
                     HashLen16(v.second, w.second) + x);
}

HMM_HASH_QUALIFIER uint64 CityHash64WithSeed(const char* s, size_t len,
                                             uint64 seed) {
    return CityHash64WithSeeds(s, len, k2, seed);
}

HMM_HASH_QUALIFIER uint128 CityHash128(const char* s, size_t len) {
    return len >= 16
               ? CityHash128WithSeed(s + 16, len - 16,
                                     uint128(Fetch64(s), Fetch64(s + 8) + k0))
               : CityHash128WithSeed(s, len, uint128(k0, k1));
}

HMM_HASH_QUALIFIER uint128 CityHash128WithSeed(const char* s, size_t len,
                                               uint128 seed) {
    if (len < 128) {
        return CityMurmur(s, len, seed);
    }

    // v, w, x, y, and z.
    std::pair<uint64, uint64> v, w;
    uint64 x = Uint128Low64(seed);
    uint64 y = Uint128High64(seed);
    uint64 z = len * k1;
    v.first = Rotate(y ^ k1, 49) * k1 + Fetch64(s);
    v.second = Rotate(v.first, 42) * k1 + Fetch64(s + 8);
    w.first = Rotate(y + z, 35) * k1 + x;
    w.second = Rotate(x + Fetch64(s + 88), 53) * k1;

    do {
        x = Rotate(x + y + v.first + Fetch64(s + 8), 37) * k1;
        y = Rotate(y + v.second + Fetch64(s + 48), 42) * k1;
        x ^= w.second;
        y += v.first + Fetch64(s + 40);
        z = Rotate(z + w.first, 33) * k1;
        v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
        w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
        std::swap(z, x);
        s += 64;
        x = Rotate(x + y + v.first + Fetch64(s + 8), 37) * k1;
        y = Rotate(y + v.second + Fetch64(s + 48), 42) * k1;
        x ^= w.second;
        y += v.first + Fetch64(s + 40);
        z = Rotate(z + w.first, 33) * k1;
        v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
        w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
        std::swap(z, x);
        s += 64;
        len -= 128;
    } while (LIKELY(len >= 128));
    x += Rotate(v.first + z, 49) * k0;
    y = y * k0 + Rotate(w.second, 37);
    z = z * k0 + Rotate(w.first, 27);
    w.first *= 9;
    v.first *= k0;
    for (size_t tail_done = 0; tail_done < len;) {
        tail_done += 32;
        y = Rotate(x + y, 42) * k0 + v.second;
        w.first += Fetch64(s + len - tail_done + 16);
        x = x * k0 + w.first;
        z += w.second + Fetch64(s + len - tail_done);
        w.second += v.first;
        v = WeakHashLen32WithSeeds(s + len - tail_done, v.first + z, v.second);
        v.first *= k0;
    }
    x = HashLen16(x, v.first);
    y = HashLen16(y + z, w.first);
    return uint128(HashLen16(x + v.second, w.second) + y,
                   HashLen16(x + w.second, y + v.second));
}

}  // namespace internal
}  // namespace hmm

#endif  // HMM_HMM_INTERNAL_CITY_HASH_IMPL_HPP
