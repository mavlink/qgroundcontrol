cmake_minimum_required(VERSION 3.25)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/modules
    ${CMAKE_SOURCE_DIR}/cmake/find-modules
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release")
endif()

set(CMAKE_REQUIRED_QUIET ON)

#######################################################
#                Custom Build Configuration
#######################################################

include(CustomOptions)

if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/custom)
    message(STATUS "Enabling custom build")
    set(QGC_CUSTOM_BUILD ON)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/custom/cmake)
    include(CustomOverrides)
endif()

#######################################################
#                   Project Info
#######################################################

# set(CMAKE_OSX_ARCHITECTURES "arm64")
# set(CMAKE_OSX_SYSROOT "iphoneos")
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
endif()

include(Git)

project(${QGC_APP_NAME}
    VERSION ${QGC_APP_VERSION}
    DESCRIPTION ${QGC_APP_DESCRIPTION}
    HOMEPAGE_URL "https://qgroundcontrol.com/"
    LANGUAGES C CXX
)

if(APPLE AND NOT IOS)
    set(MACOS TRUE)
endif()

#######################################################
#            CMake Configuration Options
#######################################################

include(GNUInstallDirs)
include(FetchContent)
include(CMakePrintHelpers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(LINUX)
        set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${CMAKE_BINARY_DIR}/AppDir/usr")
    else()
        set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${CMAKE_BINARY_DIR}/staging")
    endif()
endif()

if(QGC_USE_CACHE)
    if(CMAKE_HOST_UNIX)
        find_program(CCACHE_PROGRAM ccache)
        if(CCACHE_PROGRAM)
            message(STATUS "Using CCache")
            set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
            set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
            if(APPLE)
                set(CMAKE_OBJC_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
            endif()
            set(ENV{CCACHE_SLOPPINESS} pch_defines,time_macros,include_file_mtime,include_file_ctime)
            set(ENV{CCACHE_BASEDIR} ${CMAKE_SOURCE_DIR})
            if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                add_compile_options(-Xclang -fno-pch-timestamp)
            elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                add_compile_options(-fpch-preprocess)
            endif()
        endif()
    elseif(CMAKE_HOST_WIN32)
        find_program(SCCACHE_PROGRAM sccache)
        if(SCCACHE_PROGRAM)
            message(STATUS "Using SCCache")
            set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
            set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
        endif()
    endif()
endif()

if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
endif()

if(CMAKE_CROSSCOMPILING AND ANDROID)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIGURATION>)

# https://cmake.org/cmake/help/latest/policy/CMP0168.html#policy:CMP0168
if(POLICY CMP0168)
    cmake_policy(SET CMP0168 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0168 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0075.html#policy:CMP0075
if(POLICY CMP0075)
    cmake_policy(SET CMP0075 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0075 NEW)
endif()

if(QGC_USE_IWYU)
    include(IWYU)
endif()

#######################################################
#               Qt6 Configuration
#######################################################

if(CMAKE_CROSSCOMPILING AND NOT IS_DIRECTORY ${QT_HOST_PATH})
    message(FATAL_ERROR "You need to set QT_HOST_PATH to cross compile Qt.")
endif()

find_program(QMAKE_EXECUTABLE NAMES qmake HINTS ${QT_ROOT_DIR} ${QTDIR} ENV QTDIR PATH_SUFFIXES bin)
execute_process(COMMAND ${QMAKE_EXECUTABLE} -query QT_VERSION OUTPUT_VARIABLE QT_VERSION)
if(QT_VERSION LESS QT_MINIMUM_VERSION)
    MESSAGE(FATAL_ERROR "Minimum supported Qt version: ${QT_MINIMUM_VERSION}.
    Installed version: ${QT_VERSION}")
endif()

add_compile_definitions(QT_DISABLE_DEPRECATED_UP_TO=0x060800)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(
        NDEBUG
        QT_NO_DEBUG
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CTest)
endif()

if(ANDROID)
    list(APPEND QT_ANDROID_MULTI_ABI_FORWARD_VARS QGC_STABLE_BUILD QT_HOST_PATH)
endif()

find_package(Qt6
    ${QT_MINIMUM_VERSION}
    REQUIRED
    COMPONENTS
        Concurrent
        Core
        Core5Compat
        Location
        Multimedia
        Network
        OpenGL
        Positioning
        Qml
        QmlIntegration
        Quick
        QuickControls2
        QuickWidgets
        Sensors
        Sql
        Svg
        TextToSpeech
        Widgets
        Xml
    OPTIONAL_COMPONENTS
        Bluetooth
        Charts
        LinguistTools
        MultimediaQuickPrivate
        Quick3D
        Quick3DTools
        SerialPort
        Test
    HINTS
        ${QT_LIBRARY_HINTS}
)

if(LINUX)
    find_package(Qt6 ${QT_MINIMUM_VERSION} COMPONENTS WaylandClient)
endif()

# Set extra standard project setup options for Qt 6.7.0 and above
set(EXTRA_STANDARD_PROJECT_SETUP_OPTIONS)
if(QT_VERSION VERSION_GREATER_EQUAL 6.7.0)
    list(APPEND EXTRA_STANDARD_PROJECT_SETUP_OPTIONS I18N_SOURCE_LANGUAGE en)
endif()

qt_standard_project_setup(
    REQUIRES ${QGC_QT_MINIMUM_VERSION}
    SUPPORTS_UP_TO ${QGC_QT_MAXIMUM_VERSION}
    ${EXTRA_STANDARD_PROJECT_SETUP_OPTIONS}
)

qt_policy(
    SET QTP0001 NEW
    SET QTP0002 NEW
    SET QTP0003 NEW
    SET QTP0004 NEW
    SET QTP0005 NEW
)

#######################################################
#                QGroundControl Options
#######################################################

add_compile_definitions(
    $<$<NOT:$<BOOL:${QGC_STABLE_BUILD}>>:QGC_DAILY_BUILD>
    $<$<BOOL:${QGC_BUILD_TESTING}>:QGC_UNITTEST_BUILD>
    $<$<BOOL:${QGC_DEBUG_QML}>:QT_QML_DEBUG>
    $<$<BOOL:${QGC_DISABLE_APM_MAVLINK}>:QGC_NO_ARDUPILOT_DIALECT>
    $<$<BOOL:${QGC_VIEWER3D}>:QGC_VIEWER3D>
)

if(NOT QGC_BUILD_TESTING)
    set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
endif()

if("${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64;x86_64" OR "${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64;arm64")
    set(MACOS_UNIVERSAL_BUILD ON)
endif()

# TODO: Force building dependencies if(NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL CMAKE_SYSTEM_PROCESSOR)?
# if(CMAKE_OSX_ARCHITECTURES AND NOT "${CMAKE_HOST_SYSTEM_PROCESSOR}" IN_LIST CMAKE_OSX_ARCHITECTURES)

include(CPM)
if(CMAKE_CROSSCOMPILING OR ANDROID OR MACOS_UNIVERSAL_BUILD)
    set(CPM_DOWNLOAD_ALL ON)
endif()

#######################################################
#                Custom Build Configuration
#######################################################

if(QGC_CUSTOM_BUILD)
    add_subdirectory(custom)
endif()

#######################################################
#                QGroundControl Resources
#######################################################

# Note: Adding Resources to Library instead requires using Q_INIT_RESOURCE(qgcresources)

if(NOT QGC_CUSTOM_BUILD)
    # Custom builds can override the resources
    list(APPEND QGC_RESOURCES
        ${CMAKE_SOURCE_DIR}/qgcimages.qrc
        ${CMAKE_SOURCE_DIR}/qgcresources.qrc
        ${CMAKE_SOURCE_DIR}/qgroundcontrol.qrc
    )
endif()

list(APPEND QGC_RESOURCES
    ${CMAKE_SOURCE_DIR}/resources/InstrumentValueIcons/InstrumentValueIcons.qrc
    ${CMAKE_SOURCE_DIR}/src/FirmwarePlugin/APM/APMResources.qrc
    ${CMAKE_SOURCE_DIR}/src/FirmwarePlugin/PX4/PX4Resources.qrc
)

if(QGC_UTM_ADAPTER)
    list(APPEND QGC_RESOURCES ${CMAKE_SOURCE_DIR}/src/UTMSP/utmsp.qrc)
else()
    list(APPEND QGC_RESOURCES ${CMAKE_SOURCE_DIR}/src/UTMSP/dummy/utmsp_dummy.qrc)
endif()

if(QGC_BUILD_TESTING)
    list(APPEND QGC_RESOURCES ${CMAKE_SOURCE_DIR}/test/UnitTest.qrc)
endif()

#######################################################
#               QGroundControl Target
#######################################################

qt_add_executable(${CMAKE_PROJECT_NAME}
    WIN32
    MACOSX_BUNDLE
    src/main.cc
    ${QGC_RESOURCES}
)

if(NOT ANDROID AND NOT IOS)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            src/RunGuard.cc
            src/RunGuard.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(Qt6LinguistTools_FOUND)
    file(GLOB TS_SOURCES ${CMAKE_SOURCE_DIR}/translations/qgc_*.ts)
    set_source_files_properties(${TS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_BINARY_DIR}/i18n")
    qt_add_translations(${CMAKE_PROJECT_NAME}
        # TS_FILE_BASE ${CMAKE_PROJECT_NAME}
        # TS_FILE_DIR ${CMAKE_SOURCE_DIR}/translations/
        TS_FILES ${TS_SOURCES}
        RESOURCE_PREFIX "/"
        LUPDATE_OPTIONS -no-obsolete
    )
endif()

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        QT_RESOURCE_PREFIX "/qgc"
        OUTPUT_NAME ${CMAKE_PROJECT_NAME}
)

if(WIN32)
    # windows installer files shared with core and custom
    set(DEPLOY_WIN_FILES
        "${CMAKE_SOURCE_DIR}/deploy/windows/driver.msi"
        "${CMAKE_SOURCE_DIR}/deploy/windows/nullsoft_installer.nsi"
        "${CMAKE_SOURCE_DIR}/deploy/windows/QGroundControl.rc"
        "${QGC_WINDOWS_INSTALL_HEADER_PATH}"
        "${QGC_WINDOWS_ICON_PATH}"
    )

    # Destination directory where files will be copied
    set(QGC_INSTALLER_SOURCE_WIN "${CMAKE_BINARY_DIR}/deploy/windows")
    file(MAKE_DIRECTORY ${QGC_INSTALLER_SOURCE_WIN})
    foreach(FILE ${DEPLOY_WIN_FILES})
        # filename without the path
        get_filename_component(FILE_NAME ${FILE} NAME)
        # re-copy the file if it changes
        add_custom_command(
            OUTPUT "${QGC_INSTALLER_SOURCE_WIN}/${FILE_NAME}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE}" "${QGC_INSTALLER_SOURCE_WIN}/${FILE_NAME}"
            DEPENDS "${FILE}"  # Depend on the source file so that it re-copies when it changes
        )
        list(APPEND QGC_INSTALLER_SOURCE_WIN_FILES "${QGC_INSTALLER_SOURCE_WIN}/${FILE_NAME}")
    endforeach()

    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${QGC_INSTALLER_SOURCE_WIN_FILES})
    set_target_properties(${CMAKE_PROJECT_NAME}
        PROPERTIES
            WIN32_EXECUTABLE TRUE
            QT_TARGET_RC_ICONS "${QGC_WINDOWS_ICON_PATH}"
    )
    add_compile_definitions(_USE_MATH_DEFINES)
elseif(MACOS)
    set(MACOSX_BUNDLE_ICON_FILE "macx.icns")
    set(app_icon_macos "${QGC_MACOS_ICON_PATH}/macx.icns")
    set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${app_icon_macos})
    set_target_properties(${CMAKE_PROJECT_NAME}
        PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/deploy/macos/MacOSXBundleInfo.plist.in"
            MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_COPYRIGHT "${QGC_APP_COPYRIGHT}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "${QGC_BUNDLE_ID}"
            MACOSX_BUNDLE_ICON_FILE "macx.icns"
            MACOSX_BUNDLE_INFO_STRING "${QGC_APP_DESCRIPTION}"
            MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    )
elseif(IOS)
    enable_language(OBJC)

    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0")
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2") # iPhone,iPad
    set(CMAKE_XCODE_ATTRIBUTE_INFOPLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/deploy/ios/iOS-Info.plist")

    set_target_properties(${CMAKE_PROJECT_NAME}
        PROPERTIES
            QT_IOS_LAUNCH_SCREEN ${CMAKE_SOURCE_DIR}/deploy/ios/QGCLaunchScreen.xib
            XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.mavlink.qgroundcontrol"
            XCODE_ATTRIBUTE_PRODUCT_NAME ${CMAKE_PROJECT_NAME}
            XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION ${CMAKE_PROJECT_VERSION}
            XCODE_ATTRIBUTE_MARKETING_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
            XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
            XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
            XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone,iPad
            XCODE_ATTRIBUTE_INFOPLIST_KEY_CFBundleDisplayName ${CMAKE_PROJECT_NAME}
            XCODE_ATTRIBUTE_INFOPLIST_KEY_LSApplicationCategoryType "public.app-category.mycategory"
            XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
    )

    # set(QT_NO_FFMPEG_XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)
    qt_add_ios_ffmpeg_libraries(${CMAKE_PROJECT_NAME})
elseif(ANDROID)
    include(CPM)
    CPMAddPackage(
        NAME android_openssl
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(${CMAKE_PROJECT_NAME})

    # Generation of android version numbers must be consistent release to release such that they are always increasing

    if(${PROJECT_VERSION_MAJOR} GREATER 9)
        message(FATAL_ERROR "Major version larger than 1 digit: ${PROJECT_VERSION_MAJOR}")
    endif()
    if(${PROJECT_VERSION_MINOR} GREATER 9)
        message(FATAL_ERROR "Minor version larger than 1 digit: ${PROJECT_VERSION_MINOR}")
    endif()
    if(${PROJECT_VERSION_PATCH} GREATER 99)
        message(FATAL_ERROR "Patch version larger than 2 digits: ${PROJECT_VERSION_PATCH}")
    endif()

    # Bitness for android version number is 66/34 instead of 64/32 in because of a required version number bump screw-up ages ago
    set(ANDROID_BITNESS_CODE)
    if(${ANDROID_ABI} STREQUAL "armeabi-v7a" OR ${ANDROID_ABI} STREQUAL "x86")
        set(ANDROID_BITNESS_CODE 34)
    elseif(${ANDROID_ABI} STREQUAL "arm64-v8a" OR ${ANDROID_ABI} STREQUAL "x86_64")
        set(ANDROID_BITNESS_CODE 66)
    else()
        message(FATAL_ERROR "Unsupported Android ABI: ${ANDROID_ABI}")
    endif()

    set(ANDROID_PATCH_VERSION ${PROJECT_VERSION_PATCH})
    if(${PROJECT_VERSION_PATCH} LESS 10)
        set(ANDROID_PATCH_VERSION "0${PROJECT_VERSION_PATCH}")
    endif()

    # Version code format: BBMIPPDDD (B=Bitness, M=Major, I=Minor, P=Patch, D=Dev) - Dev not currently supported and always 000
    set(ANDROID_VERSION_CODE "${ANDROID_BITNESS_CODE}${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}${ANDROID_PATCH_VERSION}000")
    message("Android version code: ${ANDROID_VERSION_CODE}")

    set_target_properties(${CMAKE_PROJECT_NAME}
        PROPERTIES
            # QT_ANDROID_ABIS ${ANDROID_ABI}
            QT_ANDROID_MIN_SDK_VERSION ${QGC_QT_ANDROID_MIN_SDK_VERSION}
            QT_ANDROID_TARGET_SDK_VERSION ${QGC_QT_ANDROID_TARGET_SDK_VERSION}
            QT_ANDROID_PACKAGE_NAME "${QGC_ANDROID_PACKAGE_NAME}"
            QT_ANDROID_PACKAGE_SOURCE_DIR "${QGC_ANDROID_PACKAGE_SOURCE_DIR}"
            QT_ANDROID_VERSION_NAME ${CMAKE_PROJECT_VERSION}
            QT_ANDROID_VERSION_CODE ${ANDROID_VERSION_CODE}
            QT_QML_ROOT_PATH ${CMAKE_SOURCE_DIR}
            # QT_ANDROID_SDK_BUILD_TOOLS_REVISION
            # QT_ANDROID_SYSTEM_LIBS_PREFIX
    )
endif()

add_compile_definitions(
    QGC_APP_NAME="${QGC_APP_NAME}"
    QGC_ORG_NAME="${QGC_ORG_NAME}"
    QGC_ORG_DOMAIN="${QGC_ORG_DOMAIN}"
    QGC_APP_VERSION_STR="${QGC_APP_VERSION_STR}"
    QGC_SETTINGS_VERSION=${QGC_SETTINGS_VERSION}
)

add_subdirectory(src)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Core5Compat
        Qt6::Quick
        Qt6::Svg
        Qt6::Widgets
        QGC
)

if(QGC_BUILD_TESTING)
    add_subdirectory(test)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE qgctest)
endif()

if(ANDROID)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/android/src/AndroidInit.cc)

    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/android/src)
endif()

set_source_files_properties(resources/qtquickcontrols2.conf PROPERTIES QT_RESOURCE_ALIAS qtquickcontrols2.conf)
qt_add_resources(${CMAKE_PROJECT_NAME} "app_res" FILES resources/qtquickcontrols2.conf)

qt_import_plugins(${CMAKE_PROJECT_NAME}
    INCLUDE Qt6::QSvgPlugin
    EXCLUDE_BY_TYPE geoservices
    INCLUDE_BY_TYPE sqldrivers Qt6::QSQLiteDriverPlugin
)

include(Install)

include(PrintSummary)
