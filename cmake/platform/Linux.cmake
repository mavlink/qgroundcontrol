# ----------------------------------------------------------------------------
# QGroundControl Linux Platform Configuration
# ----------------------------------------------------------------------------

if(NOT LINUX)
    message(FATAL_ERROR "QGC: Invalid Platform: Linux.cmake included but platform is not Linux")
endif()

# ----------------------------------------------------------------------------
# Linux-Specific Compiler Flags
# ----------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Enable additional warnings for better code quality
    # target_compile_options(${CMAKE_PROJECT_NAME}
    #     PRIVATE
    #         -Wall
    #         -Wextra
    #         -Wpedantic
    #         $<$<CONFIG:Release>:-O3>
    # )
endif()

# ----------------------------------------------------------------------------
# Linux-Specific Definitions
# ----------------------------------------------------------------------------
target_compile_definitions(${CMAKE_PROJECT_NAME}
    PRIVATE
        _GNU_SOURCE
)

# ----------------------------------------------------------------------------
# Linux Desktop Integration
# ----------------------------------------------------------------------------
# Desktop entry and icon files are handled by the install scripts
# See cmake/install/CreateAppImage.cmake for AppImage-specific configuration

message(STATUS "QGC: Linux platform configuration applied")
