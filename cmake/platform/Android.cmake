if(NOT ANDROID)
    message(FATAL_ERROR "Invalid Platform")
    return()
endif()

if(${Qt6_VERSION} VERSION_EQUAL 6.6.3)
    if(NOT ${CMAKE_ANDROID_NDK_VERSION} VERSION_EQUAL 25.1)
        message(FATAL_ERROR "Invalid NDK Version: ${CMAKE_ANDROID_NDK_VERSION}, Use Version 25B instead.")
    endif()
elseif(${Qt6_VERSION} VERSION_EQUAL 6.8.3)
    if(NOT ${CMAKE_ANDROID_NDK_VERSION} VERSION_EQUAL 26.1)
        message(FATAL_ERROR "Invalid NDK Version: ${CMAKE_ANDROID_NDK_VERSION}, Use Version 26B instead.")
    endif()
endif()

# Generation of android version numbers must be consistent release to release such that they are always increasing
if(${CMAKE_PROJECT_VERSION_MAJOR} GREATER 9)
    message(FATAL_ERROR "Major version larger than 1 digit: ${CMAKE_PROJECT_VERSION_MAJOR}")
endif()
if(${CMAKE_PROJECT_VERSION_MINOR} GREATER 9)
    message(FATAL_ERROR "Minor version larger than 1 digit: ${CMAKE_PROJECT_VERSION_MINOR}")
endif()
if(${CMAKE_PROJECT_VERSION_PATCH} GREATER 99)
    message(FATAL_ERROR "Patch version larger than 2 digits: ${CMAKE_PROJECT_VERSION_PATCH}")
endif()

# Bitness for android version number is 66/34 instead of 64/32 in because of a required version number bump screw-up ages ago
set(ANDROID_BITNESS_CODE)
if(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "armeabi-v7a" OR ${CMAKE_ANDROID_ARCH_ABI} STREQUAL "x86")
    set(ANDROID_BITNESS_CODE 34)
elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "arm64-v8a" OR ${CMAKE_ANDROID_ARCH_ABI} STREQUAL "x86_64")
    set(ANDROID_BITNESS_CODE 66)
else()
    message(FATAL_ERROR "Unsupported Android ABI: ${CMAKE_ANDROID_ARCH_ABI}")
endif()

set(ANDROID_PATCH_VERSION ${CMAKE_PROJECT_VERSION_PATCH})
if(${CMAKE_PROJECT_VERSION_PATCH} LESS 10)
    set(ANDROID_PATCH_VERSION "0${CMAKE_PROJECT_VERSION_PATCH}")
endif()

# Version code format: BBMIPPDDD (B=Bitness, M=Major, I=Minor, P=Patch, D=Dev) - Dev not currently supported and always 000
set(ANDROID_VERSION_CODE "${ANDROID_BITNESS_CODE}${CMAKE_PROJECT_VERSION_MAJOR}${CMAKE_PROJECT_VERSION_MINOR}${ANDROID_PATCH_VERSION}000")
message(STATUS "Android version code: ${ANDROID_VERSION_CODE}")

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        # QT_ANDROID_ABIS ${CMAKE_ANDROID_ARCH_ABI}
        # QT_ANDROID_SDK_BUILD_TOOLS_REVISION
        QT_ANDROID_MIN_SDK_VERSION ${QGC_QT_ANDROID_MIN_SDK_VERSION}
        QT_ANDROID_TARGET_SDK_VERSION ${QGC_QT_ANDROID_TARGET_SDK_VERSION}
        # QT_ANDROID_COMPILE_SDK_VERSION
        QT_ANDROID_PACKAGE_NAME "${QGC_ANDROID_PACKAGE_NAME}"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${QGC_ANDROID_PACKAGE_SOURCE_DIR}"
        QT_ANDROID_VERSION_NAME "${CMAKE_PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE ${ANDROID_VERSION_CODE}
        # QT_ANDROID_APP_NAME
        # QT_ANDROID_APP_ICON
        # QT_QML_IMPORT_PATH
        QT_QML_ROOT_PATH "${CMAKE_SOURCE_DIR}"
        # QT_ANDROID_SYSTEM_LIBS_PREFIX
)

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     set(QT_ANDROID_APPLICATION_ARGUMENTS)
# endif()

list(APPEND QT_ANDROID_MULTI_ABI_FORWARD_VARS QGC_STABLE_BUILD QT_HOST_PATH)

CPMAddPackage(
    NAME android_openssl
    URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
)
include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
add_android_openssl_libraries(${CMAKE_PROJECT_NAME})
