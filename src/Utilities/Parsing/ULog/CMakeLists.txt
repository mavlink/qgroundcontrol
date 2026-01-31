# ============================================================================
# ULog Utility
# General-purpose PX4 ULog parsing utilities
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ULogUtility.cc
        ULogUtility.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# ulog_cpp Integration
# PX4 ULog parsing library
# ============================================================================

CPMAddPackage(
    NAME ulog_cpp
    GITHUB_REPOSITORY PX4/ulog_cpp
    GIT_TAG v1.0.1
)

# Suppress Clang warnings for ulog_cpp
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(ulog_cpp PRIVATE -Wno-unknown-warning-option)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ulog_cpp::ulog_cpp)
