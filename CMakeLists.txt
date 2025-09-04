cmake_minimum_required(VERSION 3.10)
project(hmm VERSION 0.1.0)

option(HMM_BUILD_TESTS "Build the tests for hmm" OFF)
option(HMM_BUILD_EXAMPLES "Build the examples for hmm" OFF)

# --- Library ---
# As a header-only library, we just need to tell CMake where to find the headers.
add_library(hmm INTERFACE)
target_include_directories(hmm INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(hmm INTERFACE cxx_std_11)

# --- Installation ---
include(GNUInstallDirs)

install(
  DIRECTORY include/hmm
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  TARGETS hmm
  EXPORT hmm-export
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
export(
  EXPORT hmm-export
  NAMESPACE hmm::
  FILE hmm-targets.cmake
)

include(CMakePackageConfigHelpers)

# Generate the version file
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/hmm-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Generate the config file
configure_package_config_file(
  "cmake/hmm-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/hmm-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hmm
)

# Install the config and version files
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hmm-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hmm-config-version.cmake"
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/hmm
)

if (HMM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif ()

if (HMM_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif ()
