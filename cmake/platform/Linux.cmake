# ----------------------------------------------------------------------------
# QGroundControl Linux Platform Configuration
# ----------------------------------------------------------------------------

if(NOT LINUX)
    message(FATAL_ERROR "QGC: Invalid Platform: Linux.cmake included but platform is not Linux")
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

set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

if(NOT DEFINED QGC_LINUX_DISTRO)
    include(LinuxDistro)
endif()

message(STATUS "QGC: Linux platform configuration applied (distro: ${QGC_LINUX_DISTRO}, family: ${QGC_LINUX_DISTRO_FAMILY})")
