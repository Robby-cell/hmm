#include <hmm/flat-hash-map.hpp>

// Std
#include <cstddef>
#include <format>
#include <iostream>
#include <memory_resource>
#include <string>

int main() {
    static std::byte buffer[2048];
    static std::pmr::monotonic_buffer_resource resource(buffer, sizeof(buffer));

    {
        // Map that uses pmr allocator
        hmm::pmr::flat_hash_map<int, int> map(
            std::pmr::polymorphic_allocator<std::byte>{&resource});

        static constexpr auto Count = 10;
        for (int i = 1; i <= Count; ++i) {
            map[i] = i * i;
        }

        std::cout << "Map contents of flat_hash_map<int, int>:\n";
        for (const auto& [k, v] : map) {
            std::cout << k << " -> " << v << '\n';
        }
    }

    {
        hmm::pmr::flat_hash_map<int, std::string> map(&resource);
        static constexpr auto Count = 10;
        for (int i = 1; i <= Count; ++i) {
            map[i] = std::format("{:x}", i * 2);
        }

        std::cout << "Map contents of flat_hash_map<int, string>:\n";
        for (const auto& [k, v] : map) {
            std::cout << k << " -> " << v << '\n';
        }
    }
}
