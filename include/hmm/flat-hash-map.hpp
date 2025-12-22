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

#ifndef HMM_HMM_FLAT_HASH_MAP_HPP
#define HMM_HMM_FLAT_HASH_MAP_HPP

#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>

#include "hmm/city-hash.hpp"
#include "hmm/internal/macros.hpp"
#include "hmm/internal/raw-hash-map.hpp"

#if HMM_HAS_CXX_17
#include <memory_resource>
#endif

namespace hmm {

template <typename K, typename V>
struct HMM_NODISCARD MapPolicy;

template <class Key, class Value, class Hash = CityHash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Alloc =
              std::allocator<typename MapPolicy<Key, Value>::slot_type>>
class flat_hash_map : protected internal::raw_hash_map<MapPolicy<Key, Value>,
                                                       Hash, KeyEqual, Alloc> {
    using Base =
        internal::raw_hash_map<MapPolicy<Key, Value>, Hash, KeyEqual, Alloc>;

   public:
    using policy_type = typename Base::policy_type;
    using hasher_type = typename Base::hasher_type;
    using key_equal = typename Base::key_equal;
    using key_type = typename Base::key_type;
    using value_type = typename Base::value_type;
    using mapped_type = typename Base::mapped_type;
    using size_type = typename Base::size_type;

    using slot_type = typename Base::slot_type;
    using slot_allocator = typename Base::slot_allocator;
    using slot_traits = typename Base::slot_traits;
    using pointer = typename Base::pointer;
    using allocator_type = typename Base::allocator_type;

    using const_iterator = typename Base::const_iterator;
    using iterator = typename Base::iterator;

    HMM_CONSTEXPR_20 flat_hash_map() = default;

    HMM_CONSTEXPR_20 flat_hash_map(
        std::initializer_list<slot_type> initial,
        const allocator_type& alloc = allocator_type())
        : Base(initial.begin(), initial.end(), alloc) {}

    template <class Iter, class Sentinel>
    HMM_CONSTEXPR_20 flat_hash_map(
        Iter begin, Sentinel end,
        const allocator_type& alloc = allocator_type())
        : Base(begin, end, alloc) {}

    HMM_CONSTEXPR_20
    explicit flat_hash_map(const allocator_type& alloc) : Base(alloc) {}

    using Base::begin;
    using Base::capacity;
    using Base::cbegin;
    using Base::cend;
    using Base::clear;
    using Base::emplace;
    using Base::empty;
    using Base::end;
    using Base::erase;
    using Base::insert;
    using Base::reserve;
    using Base::size;

    HMM_NODISCARD HMM_CONSTEXPR_20 iterator find(const key_type& key) {
        return Base::find(key);
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator
    find(const key_type& key) const {
        return Base::find(key);
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 bool contains(const key_type& key) const {
        return Base::contains(key);
    }

    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 iterator find(const K& key) {
        return Base::find(key);
    }
    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 const_iterator find(const K& key) const {
        return Base::find(key);
    }

    template <typename K>
    HMM_NODISCARD HMM_CONSTEXPR_20 bool contains(const K& key) const {
        return Base::contains(key);
    }

    template <class K, class... Args>
    HMM_CONSTEXPR_20 std::pair<iterator, bool> try_emplace(K&& key,
                                                           Args&&... args) {
        return Base::try_emplace(std::forward<K>(key),
                                 std::forward<Args>(args)...);
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& operator[](
        const key_type& key) {
        return try_emplace(key).first->second;
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& operator[](key_type&& key) {
        return try_emplace(std::move(key)).first->second;
    }

    HMM_NODISCARD HMM_CONSTEXPR_20 mapped_type& at(const key_type& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("flat_hash_map::at");
        }
        return it->second;
    }
    HMM_NODISCARD HMM_CONSTEXPR_20 const mapped_type& at(
        const key_type& key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("flat_hash_map::at");
        }
        return it->second;
    }
};

template <typename K, typename V>
struct HMM_NODISCARD MapPolicy {
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K, V>;
    using slot_type = std::pair<K, V>;

    HMM_NODISCARD static constexpr const key_type& key(
        const slot_type& pair) noexcept {
        return pair.first;
    }

    HMM_NODISCARD static constexpr const key_type& key(
        const value_type& pair) noexcept {
        return pair.first;
    }

    HMM_NODISCARD static constexpr mapped_type& value(
        slot_type& pair) noexcept {
        return pair.second;
    }

    HMM_NODISCARD static constexpr const mapped_type& value(
        const slot_type& pair) noexcept {
        return pair.second;
    }

    HMM_NODISCARD static value_type& value_from_slot(slot_type& slot) noexcept {
        return reinterpret_cast<value_type&>(slot);
    }

    HMM_NODISCARD static const value_type& value_from_slot(
        const slot_type& slot) noexcept {
        return reinterpret_cast<const value_type&>(slot);
    }

    template <class Alloc, class... Args>
    static HMM_CONSTEXPR_20 void construct(Alloc& alloc, slot_type* ptr,
                                           Args&&... args) {
        std::allocator_traits<Alloc>::construct(alloc, ptr,
                                                std::forward<Args>(args)...);
    }

    template <class Alloc>
    static HMM_CONSTEXPR_20 void destroy(Alloc& alloc, slot_type* ptr) {
        std::allocator_traits<Alloc>::destroy(alloc, ptr);
    }
};

#if HMM_HAS_CXX_17
namespace pmr {
template <class Key, class Value, class Hash = CityHash<Key>,
          class Eq = std::equal_to<Key>>
using flat_hash_map = ::hmm::flat_hash_map<
    Key, Value, Hash, Eq,
    std::pmr::polymorphic_allocator<typename MapPolicy<Key, Value>::slot_type>>;
}
#endif

}  // namespace hmm

#endif  // HMM_HMM_FLAT_HASH_MAP_HPP
