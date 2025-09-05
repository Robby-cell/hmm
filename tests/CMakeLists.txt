# --- Dependencies ---
# Add googletest
find_package(googletest QUIET)
if (NOT googletest_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG    v1.17.0
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif ()

include(GoogleTest)

# --- Tests ---
add_executable(run_tests flat_hash_map.cc flat_hash_set.cc)
target_link_libraries(run_tests PRIVATE hmm gtest_main)
set_target_properties(run_tests
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

gtest_discover_tests(run_tests)
