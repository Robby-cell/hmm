#include <cctype>
#include <functional>
#include <hmm/flat-hash-map.hpp>
#include <iostream>
#include <string>

struct Cm {
    int count{};
};

struct PersonInfo {
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

    people["John"] = PersonInfo{37, Cm{187}};
    people.insert({"Billy", {21, Cm{190}}});

    std::cout << "John is " << people.at("jOhN").age << " years old\n";
}
