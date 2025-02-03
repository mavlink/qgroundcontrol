cmake_minimum_required(VERSION 3.25)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/modules
    ${CMAKE_SOURCE_DIR}/cmake/find-modules
)

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
if(APPLE AND NOT IOS)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
endif()

include(Git)

project(${QGC_APP_NAME}
    VERSION ${QGC_APP_VERSION}
    DESCRIPTION ${QGC_APP_DESCRIPTION}
    HOMEPAGE_URL "https://qgroundcontrol.com/"
    LANGUAGES C CXX
)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    set(LINUX TRUE)
endif()

if(APPLE AND NOT IOS)
    set(MACOS TRUE)
endif()

#######################################################
#            CMake Configuration Options
#######################################################

include(GNUInstallDirs)
include(CMakeDependentOption)
include(FetchContent)
include(CMakePrintHelpers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# set(CMAKE_AUTOGEN_BETTER_GRAPH_MULTI_CONFIG ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release")
endif()

if(LINUX)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/AppDir/usr)
else()
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/staging)
endif()

if(UNIX)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Using CCache")
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        if(APPLE)
            set(CMAKE_OBJC_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        endif()
    endif()
elseif(WIN32)
    find_program(SCCACHE_PROGRAM sccache)
    if(SCCACHE_PROGRAM)
        message(STATUS "Using SCCache")
        set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
    endif()
    if(MSVC)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
            string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
            string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
        endif()
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
        if(POLICY CMP0141)
            cmake_policy(SET CMP0141 NEW)
        endif()
    endif()
endif()

if(CMAKE_CROSSCOMPILING AND ANDROID)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

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

set(QT_SILENCE_MISSING_DEPENDENCY_TARGET_WARNING ON)
set(QT_MINIMUM_VERSION 6.8.1)

find_program(QMAKE_EXECUTABLE NAMES qmake HINTS ${QT_ROOT_DIR} ${QTDIR} ENV QTDIR PATH_SUFFIXES bin)
execute_process(COMMAND ${QMAKE_EXECUTABLE} -query QT_VERSION OUTPUT_VARIABLE QT_VERSION)
if(QT_VERSION LESS QT_MINIMUM_VERSION)
    MESSAGE(FATAL_ERROR "Minimum supported Qt version: ${QT_MINIMUM_VERSION}.
    Installed version: ${QT_VERSION}")
endif()

include(Qt6QGCConfiguration)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH "Install path for QML" FORCE)
set(QML_IMPORT_PATH ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/qml ${CMAKE_BINARY_DIR}/imports CACHE STRING "Extra QML Import Paths" FORCE)
add_compile_definitions(QT_DISABLE_DEPRECATED_UP_TO=0x060800)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(
        NDEBUG
        QT_NO_DEBUG
        QT_NO_DEBUG_OUTPUT
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENABLE_TESTING ON)
    enable_testing()
    include(CTest)

    # set(QT_ENABLE_VERBOSE_DEPLOYMENT ON CACHE BOOL "Verbose Deployment")
endif()

if(ANDROID)
    cmake_print_variables(QT_ANDROID_APPLICATION_ARGUMENTS QT_HOST_PATH)

    # set(QT_USE_TARGET_ANDROID_BUILD_DIR ON CACHE BOOL "Use Target Android Build Dir" FORCE)

    list(APPEND QT_ANDROID_MULTI_ABI_FORWARD_VARS QGC_STABLE_BUILD)
    list(APPEND QT_ANDROID_MULTI_ABI_FORWARD_VARS QT_HOST_PATH)
    cmake_print_variables(QT_ANDROID_MULTI_ABI_FORWARD_VARS)
    # QT_ANDROID_DEPLOY_RELEASE
    # set(QT_ANDROID_DEPLOYMENT_TYPE ON CACHE BOOL "Deployment Type")
    cmake_dependent_option(QT_ANDROID_SIGN_APK "Enable Signing APK" ON "CMAKE_BUILD_TYPE STREQUAL Release" OFF)
    if(QT_ANDROID_SIGN_AAB)
        message(STATUS "Signing AAB")
    endif()
    if(QT_ANDROID_SIGN_APK)
        message(STATUS "Signing APK")
    endif()
    # message(STATUS "QT_ANDROID_KEYSTORE_PATH $ENV{QT_ANDROID_KEYSTORE_PATH}")
    # message(STATUS "QT_ANDROID_KEYSTORE_ALIAS $ENV{QT_ANDROID_KEYSTORE_ALIAS}")
    # QT_ANDROID_KEYSTORE_STORE_PASS, QT_ANDROID_KEYSTORE_KEY_PASS
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

qt_standard_project_setup(
    REQUIRES ${QT_MINIMUM_VERSION}
    SUPPORTS_UP_TO ${QT_MINIMUM_VERSION}
    I18N_SOURCE_LANGUAGE en
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

if(NOT QGC_STABLE_BUILD)
    add_compile_definitions(QGC_DAILY_BUILD)
endif()

cmake_dependent_option(QGC_BUILD_TESTING "Enable testing" ON "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)
if(QGC_BUILD_TESTING)
    add_compile_definitions(QGC_UNITTEST_BUILD)
else()
    set(BUILD_TESTING OFF CACHE INTERNAL "")
endif()

# option(QGC_DISABLE_MAVLINK_INSPECTOR "Disable Mavlink Inspector" OFF) # This removes QtCharts which is GPL licensed

cmake_dependent_option(QGC_DEBUG_QML "Build QGroundControl with QML debugging/profiling support." OFF "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)
if(QGC_DEBUG_QML)
    message(STATUS "To enable the QML debugger/profiler, run with: '-qmljsdebugger=port:1234'")
    add_compile_definitions(QT_QML_DEBUG)
endif()

if(QGC_DISABLE_APM_MAVLINK)
    add_compile_definitions(QGC_NO_ARDUPILOT_DIALECT)
endif()

if(QGC_VIEWER3D)
    add_compile_definitions(QGC_VIEWER3D)
endif()

if("${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64;x86_64" OR "${CMAKE_OSX_ARCHITECTURES}" MATCHES "x86_64;arm64")
    set(MACOS_UNIVERSAL_BUILD ON CACHE INTERNAL "" FORCE)
endif()

# TODO: Force building dependencies if(NOT CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL CMAKE_SYSTEM_PROCESSOR)?
# if(CMAKE_OSX_ARCHITECTURES AND NOT "${CMAKE_HOST_SYSTEM_PROCESSOR}" IN_LIST CMAKE_OSX_ARCHITECTURES)

cmake_dependent_option(QGC_BUILD_DEPENDENCIES "Force Building of Dependencies." OFF "NOT CMAKE_CROSSCOMPILING;NOT MACOS_UNIVERSAL_BUILD" ON)

include(CPM)
if(CMAKE_CROSSCOMPILING OR ANDROID OR MACOS_UNIVERSAL_BUILD)
    set(CPM_DOWNLOAD_ALL ON CACHE BOOL "Download all Dependencies" FORCE)
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
    target_sources(${PROJECT_NAME} PRIVATE ${app_icon_macos})
    set_target_properties(${PROJECT_NAME}
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
    # qt_add_ios_ffmpeg_libraries(CMAKE_PROJECT_NAME)
elseif(ANDROID)
    include(CPM)
    CPMAddPackage(
        NAME android_openssl
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(${CMAKE_PROJECT_NAME})

    set(ANDROID_PLATFORM_ARCHITECTURE_CODE)
    if(${ANDROID_ABI} STREQUAL "armeabi-v7a")
        set(ANDROID_PLATFORM_ARCHITECTURE_CODE "032")
    elseif(${ANDROID_ABI} STREQUAL "arm64-v8a")
        set(ANDROID_PLATFORM_ARCHITECTURE_CODE "064")
    elseif(${ANDROID_ABI} STREQUAL "x86")
        set(ANDROID_PLATFORM_ARCHITECTURE_CODE "132")
    elseif(${ANDROID_ABI} STREQUAL "x86_64")
        set(ANDROID_PLATFORM_ARCHITECTURE_CODE "164")
    endif()
    set(ANDROID_VERSION_CODE "${ANDROID_PLATFORM_ARCHITECTURE_CODE}${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}")

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
elseif(LINUX)

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
        Qt6::Widgets
        Qt6::Svg
        QGC
        QmlControls
)

if(QGC_BUILD_TESTING)
    add_subdirectory(test)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE qgctest)
endif()

if(ANDROID)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/android/src/AndroidInit.cc)

    target_link_libraries(${CMAKE_PROJECT_NAME}
        PRIVATE
            Qt6::Core
            Joystick
            Utilities
    )

    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/android/src)

    if(NOT QGC_NO_SERIAL_LINK)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Comms)
    endif()
endif()

qt_import_plugins(${CMAKE_PROJECT_NAME}
    INCLUDE Qt6::QSvgPlugin
    EXCLUDE_BY_TYPE geoservices
    INCLUDE_BY_TYPE sqldrivers Qt6::QSQLiteDriverPlugin
)

#######################################################
#		Install Configuration
#######################################################

include(InstallRequiredSystemLibraries)

install(
    TARGETS ${CMAKE_PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    BUNDLE  DESTINATION .
)

set(deploy_tool_options_arg "")
if(MACOS OR WIN32)
    set(deploy_tool_options_arg "-qmldir=${CMAKE_SOURCE_DIR}")
    if(MACOS_SIGNING_IDENTITY)
        message(STATUS "Signing MacOS Bundle")
        set(deploy_tool_options_arg "${deplay_tool_options_arg} -sign-for-notarization=${MACOS_SIGNING_IDENTITY}")
    endif()
endif()

qt_generate_deploy_qml_app_script(
    TARGET ${CMAKE_PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    DEPLOY_TOOL_OPTIONS ${deploy_tool_options_arg}
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})

if(ANDROID)
    # get_target_property(QGC_ANDROID_DEPLOY_FILE ${CMAKE_PROJECT_NAME} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
    # cmake_print_variables(QGC_ANDROID_DEPLOY_FILE)
elseif(LINUX)
    configure_file(
        ${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.desktop.in
        ${CMAKE_BINARY_DIR}/org.mavlink.qgroundcontrol.desktop
        @ONLY
    )
    install(
        FILES ${CMAKE_BINARY_DIR}/org.mavlink.qgroundcontrol.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    install(
        FILES ${QGC_APPIMAGE_ICON_PATH}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/
        RENAME org.mavlink.qgroundcontrol.png
    )
    configure_file(
        ${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.metainfo.xml.in
        ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
        @ONLY
    )
    install(
        FILES ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo/
    )
    install(
        FILES ${CMAKE_SOURCE_DIR}/deploy/linux/AppRun
        DESTINATION ${CMAKE_BINARY_DIR}
    )
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateAppImage.cmake")
elseif(WIN32)
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateWinInstaller.cmake")
elseif(MACOS)
    install(CODE "set(TARGET_APP_NAME ${QGC_APP_NAME})")
    install(CODE "set(MACDEPLOYQT ${Qt6_DIR}/../../../bin/macdeployqt)")
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateMacDMG.cmake")
endif()

include(printSummary)
