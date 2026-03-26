#ifndef HMM_TESTS_TEST_SHARED_HPP
#define HMM_TESTS_TEST_SHARED_HPP

#include <functional>

namespace hmm {
namespace testing {
namespace {

struct BadHash {
    size_t operator()(int /* dummy */) const {
        return 0;
    }
};

struct LifecycleTracker {
    static inline int constructions = 0;
    static inline int destructions = 0;
    int val;

    LifecycleTracker(int v) : val(v) {
        constructions++;
    }
    LifecycleTracker(const LifecycleTracker& o) : val(o.val) {
        constructions++;
    }
    LifecycleTracker(LifecycleTracker&& o) noexcept : val(o.val) {
        constructions++;
    }

    bool operator==(const LifecycleTracker& other) const {
        return val == other.val;
    }

    ~LifecycleTracker() {
        destructions++;
    }

    static void reset() {
        constructions = 0;
        destructions = 0;
    }
};

struct LifecycleHasher {
    size_t operator()(const LifecycleTracker& l) const {
        return std::hash<int>{}(l.val);
    }
};

} // namespace
} // namespace testing
} // namespace hmm

#endif // HMM_TESTS_TEST_SHARED_HPP
