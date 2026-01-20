cmake_minimum_required(VERSION 3.16)
project(VoiceChangerDemo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Platform check
if(NOT UNIX OR APPLE)
    message(FATAL_ERROR "This project only supports Linux")
endif()

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    set(ARCH_DIR "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    set(ARCH_DIR "aarch64")
else()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# External dependencies paths
set(SWITCHBOARD_SDK_DIR ${CMAKE_SOURCE_DIR}/external/switchboard-sdk)
set(SWITCHBOARD_EFFECTS_DIR ${CMAKE_SOURCE_DIR}/external/switchboard-effects)
set(SIGNALSMITH_STRETCH_DIR ${CMAKE_SOURCE_DIR}/external/signalsmith-stretch)
set(SIGNALSMITH_LINEAR_DIR ${CMAKE_SOURCE_DIR}/external/signalsmith-linear)
set(CATCH2_DIR ${CMAKE_SOURCE_DIR}/external/Catch2)

# Switchboard SDK library
add_library(SwitchboardSDK SHARED IMPORTED)
set_target_properties(SwitchboardSDK PROPERTIES
    IMPORTED_LOCATION ${SWITCHBOARD_SDK_DIR}/Release/${ARCH_DIR}/libSwitchboardSDK.so
    INTERFACE_INCLUDE_DIRECTORIES "${SWITCHBOARD_SDK_DIR}/include"
)

# Switchboard Audio Effects library
add_library(SwitchboardAudioEffects SHARED IMPORTED)
set_target_properties(SwitchboardAudioEffects PROPERTIES
    IMPORTED_LOCATION ${SWITCHBOARD_EFFECTS_DIR}/Release/${ARCH_DIR}/libSwitchboardAudioEffects.so
    INTERFACE_INCLUDE_DIRECTORIES "${SWITCHBOARD_EFFECTS_DIR}/include"
)

# Signalsmith libraries (header-only)
# signalsmith-stretch expects to find "signalsmith-linear/stft.h"
# so we need to include the parent directory
add_library(signalsmith INTERFACE)
target_include_directories(signalsmith INTERFACE
    ${SIGNALSMITH_STRETCH_DIR}
    ${CMAKE_SOURCE_DIR}/external  # For signalsmith-linear/stft.h
)

# VoiceChanger extension library
add_library(VoiceChangerExtension SHARED
    src/extension/VoiceChangerExtension.cpp
    src/nodes/PitchShiftNode.cpp
    src/nodes/RingModNode.cpp
)

target_include_directories(VoiceChangerExtension PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${SWITCHBOARD_SDK_DIR}/include
    ${SWITCHBOARD_EFFECTS_DIR}/include
)

target_link_libraries(VoiceChangerExtension PUBLIC
    SwitchboardSDK
    SwitchboardAudioEffects
    signalsmith
)

# Main demo application
add_executable(voice-changer-demo
    src/main.cpp
)

target_include_directories(voice-changer-demo PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SWITCHBOARD_SDK_DIR}/include
    ${SWITCHBOARD_EFFECTS_DIR}/include
)

target_link_libraries(voice-changer-demo PRIVATE
    VoiceChangerExtension
    SwitchboardSDK
    SwitchboardAudioEffects
)

# Set RPATH for runtime library loading
set_target_properties(voice-changer-demo PROPERTIES
    BUILD_RPATH "${SWITCHBOARD_SDK_DIR}/Release/${ARCH_DIR};${SWITCHBOARD_EFFECTS_DIR}/Release/${ARCH_DIR};${CMAKE_BINARY_DIR}"
    INSTALL_RPATH "${SWITCHBOARD_SDK_DIR}/Release/${ARCH_DIR};${SWITCHBOARD_EFFECTS_DIR}/Release/${ARCH_DIR}"
)

# Testing
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    # Add Catch2
    add_subdirectory(${CATCH2_DIR} ${CMAKE_BINARY_DIR}/Catch2)

    # Test executable
    add_executable(VoiceChangerTests
        tests/PitchShiftNodeTests.cpp
        tests/RingModNodeTests.cpp
        tests/IntegrationTests.cpp
    )

    target_include_directories(VoiceChangerTests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
        ${SWITCHBOARD_SDK_DIR}/include
        ${SWITCHBOARD_EFFECTS_DIR}/include
    )

    # Pass source directory for test asset paths
    target_compile_definitions(VoiceChangerTests PRIVATE
        TEST_ASSETS_DIR="${CMAKE_SOURCE_DIR}/test-assets"
    )

    target_link_libraries(VoiceChangerTests PRIVATE
        VoiceChangerExtension
        SwitchboardSDK
        SwitchboardAudioEffects
        Catch2::Catch2WithMain
    )

    set_target_properties(VoiceChangerTests PROPERTIES
        BUILD_RPATH "${SWITCHBOARD_SDK_DIR}/Release/${ARCH_DIR};${SWITCHBOARD_EFFECTS_DIR}/Release/${ARCH_DIR};${CMAKE_BINARY_DIR}"
    )

    # Enable CTest
    include(CTest)
    include(${CATCH2_DIR}/extras/Catch.cmake)
    catch_discover_tests(VoiceChangerTests)
endif()
