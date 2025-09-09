cmake_minimum_required(VERSION 3.15)
project(hmm VERSION 0.1.0 LANGUAGES CXX)

# Options for consumers, done perfectly.
option(HMM_BUILD_TESTS "Build the tests for hmm" OFF)
option(HMM_BUILD_EXAMPLES "Build the examples for hmm" OFF)

# =============================================================================
# 1. LIBRARY TARGET DEFINITION
# =============================================================================

# Create the INTERFACE library target. This is a "virtual" target for a
# header-only library, used to attach properties to.
add_library(hmm INTERFACE)

# (Best Practice) Create a namespaced alias. Consumers should link to hmm::hmm.
# This prevents target name collisions in larger projects.
add_library(hmm::hmm ALIAS hmm)

# Attach properties to the target. These will be inherited by anything that links to it.
target_include_directories(hmm INTERFACE
    # This path is used when building via FetchContent or add_subdirectory()
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>

    # This path is used when the installed package is found via find_package()
    $<INSTALL_INTERFACE:include>
)

# Propagate the C++ standard requirement to consumers.
target_compile_features(hmm INTERFACE cxx_std_11)


# =============================================================================
# 2. INSTALLATION AND PACKAGE EXPORT
# =============================================================================

# Use standard GNU install directories (e.g., lib, include, bin).
include(GNUInstallDirs)
# Include helpers for generating package config files.
include(CMakePackageConfigHelpers)

# Install the headers.
# This single command correctly installs the 'include/hmm' directory
# into the standard include destination (e.g., /usr/local/include/hmm).
install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install the INTERFACE target.
# For INTERFACE libraries, this command's main job is to associate the
# target 'hmm' with the export set 'hmm-targets'.
install(
    TARGETS hmm
    EXPORT hmm-targets # Name of the export set we'll create below
)

# Generate and install the 'hmm-targets.cmake' file.
# This file defines the imported target 'hmm::hmm' for consumers.
install(
    EXPORT hmm-targets
    FILE        hmm-targets.cmake
    NAMESPACE   hmm:: # The namespace must match our alias
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hmm
)

# --- Generate and install the Config and Version files ---

# Generate the version file for find_package(hmm 1.2.3 ...)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hmm-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Use the template to generate the main config file
configure_package_config_file(
    "cmake/hmm-config.cmake.in" # Assumes you have this template file
    "${CMAKE_CURRENT_BINARY_DIR}/hmm-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hmm
)

# Install the generated config files
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/hmm-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/hmm-configVersion.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/hmm
)

# =============================================================================
# 3. OPTIONAL SUBDIRECTORIES (Tests, Examples)
# =============================================================================

if (HMM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

if (HMM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()
