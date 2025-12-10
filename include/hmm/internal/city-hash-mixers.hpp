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

#ifndef HMM_HMM_INTERNAL_CITY_HASH_MIXERS_HPP
#define HMM_HMM_INTERNAL_CITY_HASH_MIXERS_HPP

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "hmm/internal/city-hash-impl.hpp"
#include "hmm/internal/macros.hpp"

namespace hmm {

// SFINAE helpers
namespace internal {
class CityHashState;

// Helper: If T is arithmetic, return ReturnType (H). Otherwise remove from
// overload set.
template <typename T, typename ReturnType>
using enable_if_arithmetic_t =
    typename std::enable_if<std::is_arithmetic<T>::value, ReturnType>::type;

// Helper: If T is enum, return ReturnType (H).
template <typename T, typename ReturnType>
using enable_if_enum_t =
    typename std::enable_if<std::is_enum<T>::value, ReturnType>::type;
}  // namespace internal

// ============================================================================
// DEFAULT HashValue OVERLOADS
// ============================================================================

// 1. Integers, Chars, Bools, Floats
template <typename H, typename T>
internal::enable_if_arithmetic_t<T, H> HashValue(H h, T t) {
    // Handling Floating Point: -0.0 and 0.0 must hash identically
    if HMM_CONSTEXPR_17 (std::is_floating_point<T>::value) {
        if (t == 0) {
            t = 0;
        }
    }
    return H::combine_bytes(std::move(h), &t, sizeof(t));
}

// 2. Enums
template <typename H, typename T>
internal::enable_if_enum_t<T, H> HashValue(H h, T t) {
    return H::combine_bytes(std::move(h), &t, sizeof(t));
}

// 3. Pointers
template <typename H, typename T>
H HashValue(H h, T* ptr) {
    // We hash the address itself
    auto addr = reinterpret_cast<std::uintptr_t>(ptr);
    return H::combine_bytes(std::move(h), &addr, sizeof(addr));
}

// 4. std::string
template <typename H>
H HashValue(H h, const std::string& s) {
    return H::combine_contiguous(std::move(h), s.data(), s.size());
}

// 5. std::pair
template <typename H, typename T1, typename T2>
H HashValue(H h, const std::pair<T1, T2>& p) {
    return H::combine(std::move(h), p.first, p.second);
}

// 6. std::vector (Optimized Contiguous Path for arithmetic types)
template <typename H, typename T>
typename std::enable_if<std::is_arithmetic<T>::value, H>::type HashValue(
    H h, const std::vector<T>& v) {
    return H::combine_contiguous(std::move(h), v.data(), v.size());
}

// 7. std::vector (Generic iteration for complex types)
template <typename H, typename T>
typename std::enable_if<!std::is_arithmetic<T>::value, H>::type HashValue(
    H h, const std::vector<T>& v) {
    for (const auto& item : v) {
        h = H::combine(std::move(h), item);
    }
    return h;
}

namespace internal {

class CityHashState {
   public:
    // Factory
    static CityHashState Create() {
        return CityHashState();
    }

    // Base case for recursion
    static CityHashState combine(CityHashState h) {
        return h;
    }

    // ------------------------------------------------------------------------
    // COMBINE: The Variadic Entry Point
    // ------------------------------------------------------------------------
    template <typename T, typename... Args>
    static CityHashState combine(CityHashState h, const T& v,
                                 const Args&... args) {
        // Recurse: Hash head, then tail.
        // HashValue now correctly returns CityHashState, so this compiles.
        return CityHashState::combine(HashValue(std::move(h), v), args...);
    }

    // ========================================================================
    // LOW LEVEL MIXERS
    // ========================================================================

    // Mix arbitrary bytes (Used for objects with unique representation)
    static CityHashState combine_bytes(CityHashState h, const void* ptr,
                                       size_t len) {
        h.state_ =
            CityHash64WithSeed(static_cast<const char*>(ptr), len, h.state_);
        return h;
    }

    // Contiguous optimization (Used for vector, string, array)
    template <typename T>
    static CityHashState combine_contiguous(CityHashState h, const T* ptr,
                                            size_t count) {
        return combine_bytes(std::move(h), ptr, count * sizeof(T));
    }

    // Extract result
    uint64_t finalize() const {
        return state_;
    }

   private:
    CityHashState() : state_(k2) {}  // Initial seed
    uint64_t state_;
};

}  // namespace internal
}  // namespace hmm

#endif  // HMM_HMM_INTERNAL_CITY_HASH_MIXERS_HPP
