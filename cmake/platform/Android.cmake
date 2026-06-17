# ----------------------------------------------------------------------------
# QGroundControl Android Platform Configuration
# ----------------------------------------------------------------------------

if(NOT ANDROID)
    message(FATAL_ERROR "QGC: Invalid Platform: Android.cmake included but platform is not Android")
endif()

# ----------------------------------------------------------------------------
# Android NDK Version Validation
# ----------------------------------------------------------------------------
# CMAKE_ANDROID_NDK_VERSION format varies: "27.2" or "27.2.12829759"
# Extract major.minor from ndk_full_version for reliable comparison
if(DEFINED QGC_CONFIG_NDK_FULL_VERSION)
    string(REGEX MATCH "^([0-9]+\\.[0-9]+)" _ndk_major_minor "${QGC_CONFIG_NDK_FULL_VERSION}")
    if(_ndk_major_minor AND NOT CMAKE_ANDROID_NDK_VERSION VERSION_GREATER_EQUAL "${_ndk_major_minor}")
        message(FATAL_ERROR "QGC: NDK ${CMAKE_ANDROID_NDK_VERSION} is too old. Qt ${Qt6_VERSION} requires NDK ${_ndk_major_minor}+ (${QGC_CONFIG_NDK_VERSION})")
    endif()
    unset(_ndk_major_minor)
endif()

# ----------------------------------------------------------------------------
# Android Version Number Validation
# ----------------------------------------------------------------------------

# Generation of Android version numbers must be consistent release to release
# to ensure they are always increasing for Google Play Store
if(CMAKE_PROJECT_VERSION_MAJOR GREATER 9)
    message(FATAL_ERROR "QGC: Major version must be single digit (0-9), got: ${CMAKE_PROJECT_VERSION_MAJOR}")
endif()
if(CMAKE_PROJECT_VERSION_MINOR GREATER 9)
    message(FATAL_ERROR "QGC: Minor version must be single digit (0-9), got: ${CMAKE_PROJECT_VERSION_MINOR}")
endif()
if(CMAKE_PROJECT_VERSION_PATCH GREATER 99)
    message(FATAL_ERROR "QGC: Patch version must be two digits (0-99), got: ${CMAKE_PROJECT_VERSION_PATCH}")
endif()

# ----------------------------------------------------------------------------
# Android ABI to Bitness Code Mapping
# ----------------------------------------------------------------------------
# NOTE: Bitness codes are 66/34 instead of 64/32 due to a historical
# version number bump requirement from an earlier Android release
set(ANDROID_BITNESS_CODE)
if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a" OR CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
    set(ANDROID_BITNESS_CODE 34)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a" OR CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    set(ANDROID_BITNESS_CODE 66)
else()
    message(FATAL_ERROR "QGC: Unsupported Android ABI: ${CMAKE_ANDROID_ARCH_ABI}. Supported: armeabi-v7a, arm64-v8a, x86, x86_64")
endif()

# ----------------------------------------------------------------------------
# Android Version Code Generation
# ----------------------------------------------------------------------------
# Zero-pad patch version if less than 10
set(ANDROID_PATCH_VERSION ${CMAKE_PROJECT_VERSION_PATCH})
if(CMAKE_PROJECT_VERSION_PATCH LESS 10)
    set(ANDROID_PATCH_VERSION "0${CMAKE_PROJECT_VERSION_PATCH}")
endif()

# Dev field (last 3 digits) = commits since tag, so daily builds get unique,
# increasing codes; tagged releases stay at 000. Clamp at the field width (999).
set(ANDROID_DEV_VERSION "${QGC_APP_VERSION_DEV}")
if(NOT ANDROID_DEV_VERSION MATCHES "^[0-9]+$")
    set(ANDROID_DEV_VERSION 0)
endif()
if(ANDROID_DEV_VERSION GREATER 999)
    message(WARNING "QGC: Android dev version ${ANDROID_DEV_VERSION} exceeds 999; clamping. Cut a release tag to reset the counter.")
    set(ANDROID_DEV_VERSION 999)
endif()
string(LENGTH "${ANDROID_DEV_VERSION}" _qgc_dev_len)
if(_qgc_dev_len EQUAL 1)
    set(ANDROID_DEV_VERSION "00${ANDROID_DEV_VERSION}")
elseif(_qgc_dev_len EQUAL 2)
    set(ANDROID_DEV_VERSION "0${ANDROID_DEV_VERSION}")
endif()

# Version code format: BBMIPPDDD (B=Bitness, M=Major, I=Minor, P=Patch, D=Dev)
set(ANDROID_VERSION_CODE "${ANDROID_BITNESS_CODE}${CMAKE_PROJECT_VERSION_MAJOR}${CMAKE_PROJECT_VERSION_MINOR}${ANDROID_PATCH_VERSION}${ANDROID_DEV_VERSION}")
message(STATUS "QGC: Android version code: ${ANDROID_VERSION_CODE}")

# ----------------------------------------------------------------------------
# Extra Java Sources (CPM-deployed dependencies)
# ----------------------------------------------------------------------------
# CPM Java dependencies are copied into this directory by src/Android/CMakeLists.txt
# and picked up by android/build.gradle as a supplementary source set. Gradle
# derives the same location as the sibling 'extra_java_sources' of its project dir.
set(QGC_ANDROID_EXTRA_JAVA_SOURCES_DIR "${CMAKE_BINARY_DIR}/extra_java_sources")

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        # QT_ANDROID_ABIS ${CMAKE_ANDROID_ARCH_ABI}
        # QT_ANDROID_SDK_BUILD_TOOLS_REVISION
        QT_ANDROID_MIN_SDK_VERSION ${QGC_QT_ANDROID_MIN_SDK_VERSION}
        QT_ANDROID_TARGET_SDK_VERSION ${QGC_QT_ANDROID_TARGET_SDK_VERSION}
        QT_ANDROID_COMPILE_SDK_VERSION ${QGC_QT_ANDROID_COMPILE_SDK_VERSION}
        QT_ANDROID_PACKAGE_NAME "${QGC_ANDROID_PACKAGE_NAME}"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${QGC_ANDROID_PACKAGE_SOURCE_DIR}"
        QT_ANDROID_VERSION_NAME "${CMAKE_PROJECT_VERSION}"
        QT_ANDROID_VERSION_CODE ${ANDROID_VERSION_CODE}
        QT_ANDROID_APP_NAME "${CMAKE_PROJECT_NAME}"
        QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
        QT_ANDROID_LEGACY_PACKAGING $<BOOL:${QGC_ENABLE_ASAN}>
        QT_QML_ROOT_PATH "${CMAKE_SOURCE_DIR}"
        # QT_QML_IMPORT_PATH
        # QT_ANDROID_SYSTEM_LIBS_PREFIX
)

# set(QT_ANDROID_POST_BUILD_GRADLE_CLEANUP ON)

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     set(QT_ANDROID_APPLICATION_ARGUMENTS)
# endif()

# Forward Python3_EXECUTABLE so per-ABI sub-configures use the same interpreter (jinja2 lives in workspace .venv, not hostedtoolcache python).
list(APPEND QT_ANDROID_MULTI_ABI_FORWARD_VARS QGC_STABLE_BUILD QT_HOST_PATH Python3_EXECUTABLE)

# ----------------------------------------------------------------------------
# Android OpenSSL Libraries
# ----------------------------------------------------------------------------
include(AndroidOpenSSL)

# ----------------------------------------------------------------------------
# Android Permissions
# ----------------------------------------------------------------------------

qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.BLUETOOTH_SCAN
    ATTRIBUTES
        minSdkVersion 31
        usesPermissionFlags neverForLocation
)
qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.BLUETOOTH_CONNECT
    ATTRIBUTES
        minSdkVersion 31
        usesPermissionFlags neverForLocation
)

# Need MulticastLock to receive broadcast UDP packets
qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.CHANGE_WIFI_MULTICAST_STATE
)

# Needed for read/write to SD Card Path in AppSettings
qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.WRITE_EXTERNAL_STORAGE
    ATTRIBUTES
        maxSdkVersion 32
)
qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.READ_EXTERNAL_STORAGE
    ATTRIBUTES
        maxSdkVersion 33
)

option(QGC_ANDROID_ENABLE_MANAGE_EXTERNAL_STORAGE "Request MANAGE_EXTERNAL_STORAGE (not Play Store compliant by default)" OFF)
if(QGC_ANDROID_ENABLE_MANAGE_EXTERNAL_STORAGE)
    qt_add_android_permission(${CMAKE_PROJECT_NAME}
        NAME android.permission.MANAGE_EXTERNAL_STORAGE
    )
endif()

# Joystick
qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.VIBRATE
)

qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.INTERNET
)
qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.WAKE_LOCK
)
qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.ACCESS_NETWORK_STATE
)

qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.ACCESS_FINE_LOCATION
)
qt_add_android_permission(${CMAKE_PROJECT_NAME}
    NAME android.permission.ACCESS_COARSE_LOCATION
)

message(STATUS "QGC: Android platform configuration applied")
