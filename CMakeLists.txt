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
# set(CMAKE_POLICY_VERSION_MINIMUM 3.15)

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

    # CMAKE_APPLE_SILICON_PROCESSOR
    if("${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64;x86_64" OR "${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64;arm64")
        set(MACOS_UNIVERSAL_BUILD ON)
    endif()
endif()

#######################################################
#            CMake Configuration Options
#######################################################

include(GNUInstallDirs)
include(FetchContent)
include(CMakePrintHelpers)
include(CPM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_BUILD_DATABASE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
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
            # set(ENV{CCACHE_SLOPPINESS} pch_defines,time_macros,include_file_mtime,include_file_ctime)
            # set(ENV{CCACHE_BASEDIR} ${CMAKE_SOURCE_DIR})
            # if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            #     add_compile_options(-Xclang -fno-pch-timestamp)
            # elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            #     add_compile_options(-fpch-preprocess)
            # endif()
        endif()
    elseif(CMAKE_HOST_WIN32)
        find_program(SCCACHE_PROGRAM sccache)
        if(SCCACHE_PROGRAM)
            message(STATUS "Using SCCache")
            set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
            set(CMAKE_C_LINKER_LAUNCHER "${SCCACHE_PROGRAM}")
            set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
            set(CMAKE_CXX_LINKER_LAUNCHER "${SCCACHE_PROGRAM}")
        endif()
    endif()
endif()

if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
endif()

if(CMAKE_CROSSCOMPILING)
    # CMAKE_CROSSCOMPILING_EMULATOR
    # CMAKE_HOST_SYSTEM_PROCESSOR
    if(ANDROID)
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
        set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    endif()

    if(NOT IS_DIRECTORY ${QT_HOST_PATH})
        message(FATAL_ERROR "You need to set QT_HOST_PATH to cross compile Qt.")
    endif()
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

#######################################################
#               Qt6 Configuration
#######################################################

find_program(QMAKE_EXECUTABLE NAMES qmake6 HINTS ${QT_ROOT_DIR} ${QTDIR} ENV QTDIR PATH_SUFFIXES bin)
execute_process(COMMAND ${QMAKE_EXECUTABLE} -query QT_VERSION OUTPUT_VARIABLE QT_VERSION)
if(QT_VERSION LESS QT_MINIMUM_VERSION)
    message(FATAL_ERROR "Minimum supported Qt version: ${QT_MINIMUM_VERSION}. Installed version: ${QT_VERSION}")
endif()

find_package(Qt6
    ${QT_MINIMUM_VERSION}
    REQUIRED
    COMPONENTS
        Charts
        Concurrent
        Core
        Core5Compat
        Gui
        Location
        LinguistTools
        Multimedia
        MultimediaQuickPrivate
        Network
        Positioning
        Qml
        QmlIntegration
        Quick
        QuickControls2
        Sensors
        Sql
        Svg
        TextToSpeech
        Widgets
        Xml
)

if(QGC_BUILD_TESTING)
    find_package(Qt6 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS Test)
endif()

if(QGC_ENABLE_GST_VIDEOSTREAMING)
    find_package(Qt6 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS OpenGL ShaderTools)
endif()

if(QGC_ENABLE_BLUETOOTH)
    find_package(Qt6 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS Bluetooth)
endif()

if(QGC_VIEWER3D)
    find_package(Qt6 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS Quick3D)
endif()

if(NOT ANDROID AND NOT IOS)
    find_package(Qt6 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS SerialPort)
endif()

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

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(QGC_DEFINES
        ${QGC_DEFINES}
        NDEBUG
        QT_NO_DEBUG
        CACHE INTERNAL "" FORCE
    )
else()
    set(QGC_DEFINES
        ${QGC_DEFINES}
        QT_DISABLE_DEPRECATED_UP_TO=0x060800 # QGC_QT_MINIMUM_VERSION
        QT_ENABLE_STRICT_MODE_UP_TO=0x060600 # QGC_QT_MINIMUM_VERSION, QT_VERSION_CHECK, QT_VERSION_STR, QT_VERSION
        CACHE INTERNAL "" FORCE
    )
endif()

set(QGC_DEFINES
    ${QGC_DEFINES}
    QGC_APP_NAME="${QGC_APP_NAME}"
    QGC_ORG_NAME="${QGC_ORG_NAME}"
    QGC_ORG_DOMAIN="${QGC_ORG_DOMAIN}"
    QGC_APP_VERSION_STR="${QGC_APP_VERSION_STR}"
    QGC_SETTINGS_VERSION=${QGC_SETTINGS_VERSION}
    CACHE INTERNAL "" FORCE
)

if(NOT QGC_STABLE_BUILD)
    set(QGC_DEFINES
        ${QGC_DEFINES}
        QGC_DAILY_BUILD
        CACHE INTERNAL "" FORCE
    )
endif()

if(QGC_DEBUG_QML)
    set(QGC_DEFINES
        ${QGC_DEFINES}
        QT_QML_DEBUG
        CACHE INTERNAL "" FORCE
    )
endif()

if(QGC_BUILD_TESTING)
    set(QGC_DEFINES
        ${QGC_DEFINES}
        QGC_UNITTEST_BUILD
        CACHE INTERNAL "" FORCE
    )
else()
    set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
endif()

set(QGC_PROPERTIES
    ${QGC_PROPERTIES}
    QT_RESOURCE_PREFIX "/"
    OUTPUT_NAME ${CMAKE_PROJECT_NAME}
    CACHE INTERNAL "" FORCE
)

#######################################################
#                Custom Build Configuration
#######################################################

if(QGC_CUSTOM_BUILD)
    add_subdirectory(custom)
else()
    set(QGC_RESOURCES
        ${QGC_RESOURCES}
        ${CMAKE_SOURCE_DIR}/qgcimages.qrc
        ${CMAKE_SOURCE_DIR}/qgcresources.qrc
        ${CMAKE_SOURCE_DIR}/qgroundcontrol.qrc
        CACHE INTERNAL "" FORCE
    )
endif()

#######################################################
#                QGroundControl Resources
#######################################################

# Note: Adding Resources to Library instead requires using Q_INIT_RESOURCE(qgcresources)

set(QGC_RESOURCES
    ${QGC_RESOURCES}
    ${CMAKE_SOURCE_DIR}/resources/InstrumentValueIcons/InstrumentValueIcons.qrc
    ${CMAKE_SOURCE_DIR}/src/FirmwarePlugin/APM/APMResources.qrc
    ${CMAKE_SOURCE_DIR}/src/FirmwarePlugin/PX4/PX4Resources.qrc
    CACHE INTERNAL "" FORCE
)

if(QGC_UTM_ADAPTER)
    set(QGC_RESOURCES
        ${QGC_RESOURCES}
        ${CMAKE_SOURCE_DIR}/src/UTMSP/utmsp.qrc
        CACHE INTERNAL "" FORCE
    )
else()
    set(QGC_RESOURCES
        ${QGC_RESOURCES}
        ${CMAKE_SOURCE_DIR}/src/UTMSP/dummy/utmsp_dummy.qrc
        CACHE INTERNAL "" FORCE
    )
endif()

#######################################################
#               QGroundControl Target
#######################################################

if(WIN32)
    # if(QGC_WINDOWS_ICON_PATH MATCHES ".in$")
    #     configure_file(
    #         ${QGC_WINDOWS_ICON_PATH}
    #         ${CMAKE_BINARY_DIR}/metainfo/${QGC_PACKAGE_NAME}.metainfo.xml
    #         @ONLY
    #     )

    # windows installer files shared with core and custom
    set(DEPLOY_WIN_FILES
        "${CMAKE_SOURCE_DIR}/deploy/windows/driver.msi"
        "${CMAKE_SOURCE_DIR}/deploy/windows/nullsoft_installer.nsi"
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

    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${QGC_INSTALLER_SOURCE_WIN_FILES}
        CACHE INTERNAL "" FORCE
    )
    set(QGC_PROPERTIES
        ${QGC_PROPERTIES}
        WIN32_EXECUTABLE TRUE
        CACHE INTERNAL "" FORCE
    )
    # set(QGC_PROPERTIES
    #     ${QGC_PROPERTIES}
    #     QT_TARGET_RC_ICONS "${QGC_WINDOWS_ICON_PATH}"
    #     QT_TARGET_COMPANY_NAME: RC Company name
    #     QT_TARGET_DESCRIPTION: RC File Description
    #     QT_TARGET_VERSION: RC File and Product Version
    #     QT_TARGET_COPYRIGHT: RC LegalCopyright
    #     QT_TARGET_PRODUCT_NAME: RC ProductName
    #     QT_TARGET_COMMENTS: RC Comments
    #     QT_TARGET_ORIGINAL_FILENAME: RC Original FileName
    #     QT_TARGET_TRADEMARKS: RC LegalTrademarks
    #     QT_TARGET_INTERNALNAME: RC InternalName
    #     QT_TARGET_RC_ICONS: List of paths to icon files
    #     CACHE INTERNAL "" FORCE
    # )
    set(QGC_DEFINES
        ${QGC_DEFINES}
        _USE_MATH_DEFINES
        CACHE INTERNAL "" FORCE
    )
elseif(MACOS)
    set(MACOSX_BUNDLE_ICON_FILE macx.icns)
    set(app_icon_macos "${QGC_MACOS_ICON_PATH}/${MACOSX_BUNDLE_ICON_FILE}")
    set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${app_icon_macos}
        CACHE INTERNAL "" FORCE
    )
    set(QGC_PROPERTIES
        ${QGC_PROPERTIES}
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/deploy/macos/MacOSXBundleInfo.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "${QGC_APP_COPYRIGHT}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${QGC_BUNDLE_ID}"
        MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
        MACOSX_BUNDLE_INFO_STRING "${QGC_APP_DESCRIPTION}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}"
        CACHE INTERNAL "" FORCE
    )
elseif(IOS)
    enable_language(OBJC)

    set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0")
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2") # iPhone,iPad
    set(CMAKE_XCODE_ATTRIBUTE_INFOPLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/deploy/ios/iOS-Info.plist")

    set(QGC_PROPERTIES
        ${QGC_PROPERTIES}
        QT_IOS_LAUNCH_SCREEN ${CMAKE_SOURCE_DIR}/deploy/ios/QGCLaunchScreen.xib
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${QGC_PACKAGE_NAME}"
        XCODE_ATTRIBUTE_PRODUCT_NAME ${CMAKE_PROJECT_NAME}
        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION ${CMAKE_PROJECT_VERSION}
        XCODE_ATTRIBUTE_MARKETING_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "14.0"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone,iPad
        XCODE_ATTRIBUTE_INFOPLIST_KEY_CFBundleDisplayName ${CMAKE_PROJECT_NAME}
        XCODE_ATTRIBUTE_INFOPLIST_KEY_LSApplicationCategoryType "public.app-category.mycategory"
        XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS "YES"
        CACHE INTERNAL "" FORCE
    )
elseif(ANDROID)
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
    if(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "armeabi-v7a" OR ${CMAKE_ANDROID_ARCH_ABI} STREQUAL "x86")
        set(ANDROID_BITNESS_CODE 34)
    elseif(${CMAKE_ANDROID_ARCH_ABI} STREQUAL "arm64-v8a" OR ${CMAKE_ANDROID_ARCH_ABI} STREQUAL "x86_64")
        set(ANDROID_BITNESS_CODE 66)
    else()
        message(FATAL_ERROR "Unsupported Android ABI: ${CMAKE_ANDROID_ARCH_ABI}")
    endif()

    set(ANDROID_PATCH_VERSION ${PROJECT_VERSION_PATCH})
    if(${PROJECT_VERSION_PATCH} LESS 10)
        set(ANDROID_PATCH_VERSION "0${PROJECT_VERSION_PATCH}")
    endif()

    # Version code format: BBMIPPDDD (B=Bitness, M=Major, I=Minor, P=Patch, D=Dev) - Dev not currently supported and always 000
    set(ANDROID_VERSION_CODE "${ANDROID_BITNESS_CODE}${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}${ANDROID_PATCH_VERSION}000")
    message(NOTICE "Android version code: ${ANDROID_VERSION_CODE}")

    set(QGC_PROPERTIES
        ${QGC_PROPERTIES}
        # QT_ANDROID_ABIS ${CMAKE_ANDROID_ARCH_ABI}
        # QT_ANDROID_SDK_BUILD_TOOLS_REVISION
        QT_ANDROID_MIN_SDK_VERSION ${QGC_QT_ANDROID_MIN_SDK_VERSION}
        QT_ANDROID_TARGET_SDK_VERSION ${QGC_QT_ANDROID_TARGET_SDK_VERSION}
        # QT_ANDROID_COMPILE_SDK_VERSION
        QT_ANDROID_PACKAGE_NAME "${QGC_ANDROID_PACKAGE_NAME}"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${QGC_ANDROID_PACKAGE_SOURCE_DIR}"
        QT_ANDROID_VERSION_NAME ${CMAKE_PROJECT_VERSION}
        QT_ANDROID_VERSION_CODE ${ANDROID_VERSION_CODE}
        # QT_ANDROID_APP_NAME
        # QT_ANDROID_APP_ICON
        # QT_QML_IMPORT_PATH
        QT_QML_ROOT_PATH ${CMAKE_SOURCE_DIR}
        # QT_ANDROID_SYSTEM_LIBS_PREFIX
        CACHE INTERNAL "" FORCE
    )

    # if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    #     set(QT_ANDROID_APPLICATION_ARGUMENTS)
    # endif()

    list(APPEND QT_ANDROID_MULTI_ABI_FORWARD_VARS QGC_STABLE_BUILD QT_HOST_PATH)
endif()

add_subdirectory(src)
if(QGC_BUILD_TESTING)
    add_subdirectory(test)
endif()

qt_add_executable(${CMAKE_PROJECT_NAME}
    WIN32
    MACOSX_BUNDLE
    ${QGC_SOURCES}
    ${QGC_RESOURCES}
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${QGC_LIBRARIES})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${QGC_DIRECTORIES})
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ${QGC_DEFINES})
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ${QGC_PROPERTIES})

target_precompile_headers(${CMAKE_PROJECT_NAME}
    PRIVATE
        <QtCore/QObject>
        <QtNetwork/QNetworkReply>
        <QtPositioning/QGeoCoordinate>
)

file(GLOB TS_SOURCES ${CMAKE_SOURCE_DIR}/translations/qgc_*.ts)
set_source_files_properties(${TS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_BINARY_DIR}/i18n")
qt_add_translations(${CMAKE_PROJECT_NAME}
    # TS_FILE_BASE ${CMAKE_PROJECT_NAME}
    # TS_FILE_DIR ${CMAKE_SOURCE_DIR}/translations/
    TS_FILES ${TS_SOURCES}
    RESOURCE_PREFIX "/"
    LUPDATE_OPTIONS -no-obsolete
)

set_source_files_properties(resources/qtquickcontrols2.conf PROPERTIES QT_RESOURCE_ALIAS qtquickcontrols2.conf)
qt_add_resources(${CMAKE_PROJECT_NAME} "qgcresources_cmake"
    PREFIX "/"
    FILES
        resources/qtquickcontrols2.conf
)

qt_import_plugins(${CMAKE_PROJECT_NAME}
    INCLUDE Qt6::QSvgPlugin
    EXCLUDE_BY_TYPE geoservices
    INCLUDE_BY_TYPE sqldrivers Qt6::QSQLiteDriverPlugin
)

include(Install)

include(PrintSummary)
