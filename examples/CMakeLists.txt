add_executable(example main.cc)
target_link_libraries(example PRIVATE hmm)
set_target_properties(example
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
