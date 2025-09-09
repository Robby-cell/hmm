add_executable(example main.cc)
target_link_libraries(example PRIVATE hmm)
set_target_properties(example
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)

add_executable(c++20-example c++20-example.cc)
target_link_libraries(c++20-example PRIVATE hmm)
set_target_properties(c++20-example
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
