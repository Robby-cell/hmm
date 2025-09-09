#include <hmm/flat-hash-map.hpp>
#include <hmm/flat-hash-set.hpp>

// Std
#include <cctype>
#include <functional>
#include <iostream>
#include <string>

struct Cm {
    Cm() = default;
    Cm(const Cm&) = default;
    Cm(Cm&&) noexcept = default;

    Cm& operator=(const Cm&) = default;
    Cm& operator=(Cm&&) noexcept = default;

    Cm(int v) : count(v) {}

    int count{};
};

struct PersonInfo {
    PersonInfo() = default;
    PersonInfo(int age, Cm height) : age(age), height(height) {}

    PersonInfo(const PersonInfo&) = default;
    PersonInfo(PersonInfo&&) noexcept = default;

    PersonInfo& operator=(const PersonInfo&) = default;
    PersonInfo& operator=(PersonInfo&&) noexcept = default;

    int age{};
    Cm height{};
};

struct IgnoreCaseHash {
    std::size_t operator()(std::string input) const {
        // Just take a copy and modify it in place, to show that a hash function
        // can be created
        for (auto& c : input) {
            c = std::tolower(c);
        }
        return std::hash<std::string>{}(input);
    }
};

struct IgnoreCaseCmp {
    bool operator()(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (auto it1 = a.begin(), it2 = b.begin(), end = a.end(); it1 != end;
             ++it1, ++it2) {
            if (std::tolower(*it1) != std::tolower(*it2)) {
                return false;
            }
        }
        return true;
    }
};

int main() {
    hmm::flat_hash_map<std::string, PersonInfo, IgnoreCaseHash, IgnoreCaseCmp>
        people;

    people["John"] = PersonInfo{37, Cm(187)};
    people.insert({"Billy", {21, Cm{190}}});

    std::cout << "John is " << people.at("jOhN").age << " years old\n";

    hmm::flat_hash_set<std::string> info;
    for (const auto& pair : people) {
        auto&& name = pair.first;
        auto&& person_info = pair.second;

        info.insert(name + " is " + std::to_string(person_info.age));
    }

    for (const auto& entry : info) {
        std::cout << entry << '\n';
    }
}
