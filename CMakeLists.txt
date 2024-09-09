cmake_minimum_required(VERSION 3.22.1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

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

include(Git)

project(QGroundControl
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

if(IOS OR ANDROID)
    set(MOBILE TRUE)
    add_compile_definitions(__mobile__)
endif()

#######################################################
#            CMake Configuration Options
#######################################################

include(GNUInstallDirs)
include(CMakeDependentOption)
include(FetchContent)
include(CMakePrintHelpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR OFF)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
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
    endif()
elseif(WIN32)
    find_program(SCCACHE_PROGRAM sccache)
    if(SCCACHE_PROGRAM)
        message(STATUS "Using SCCache")
        set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
        if(MSVC)
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
                string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
            elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
                string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
                string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
            endif()
            set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded)
        endif()
    endif()
endif()

set(BUILD_SHARED_LIBS OFF)

# include(CMakePrintSystemInformation)
# include(ProcessorCount)
cmake_print_variables(
    CMAKE_VERSION
    CMAKE_PREFIX_PATH
    CMAKE_HOST_SYSTEM_NAME
    CMAKE_HOST_SYSTEM_VERSION
    CMAKE_SYSTEM_NAME
    CMAKE_SYSTEM_VERSION
    CMAKE_CURRENT_SOURCE_DIR
    CMAKE_BUILD_TYPE
    CMAKE_TOOLCHAIN_FILE
)

#######################################################
#               Qt6 Configuration
#######################################################

include(Qt6QGCConfiguration)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH "Install path for QML" FORCE)
# set(QML_IMPORT_PATH ${CMAKE_SOURCE_DIR}/qml ${CMAKE_BINARY_DIR}/imports CACHE STRING "Extra QML Import Paths" FORCE)
add_compile_definitions(QT_DISABLE_DEPRECATED_UP_TO=0x060600)

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

    set(QT_ENABLE_VERBOSE_DEPLOYMENT ON CACHE BOOL "Verbose Deployment")
endif()

if(ANDROID)
    cmake_print_variables(QT_ANDROID_APPLICATION_ARGUMENTS QT_HOST_PATH)

    # QT_USE_TARGET_ANDROID_BUILD_DIR
    list(APPEND QT_ANDROID_MULTI_ABI_FORWARD_VARS QGC_STABLE_BUILD)
    list(APPEND QT_ANDROID_MULTI_ABI_FORWARD_VARS QT_HOST_PATH)
    cmake_print_variables(QT_ANDROID_MULTI_ABI_FORWARD_VARS)
    # QT_ANDROID_DEPLOY_RELEASE
    # set(QT_ANDROID_DEPLOYMENT_TYPE ON CACHE BOOL "Deployment Type")
    cmake_dependent_option(QT_ANDROID_SIGN_APK "Enable Signing APK" ON "CMAKE_BUILD_TYPE STREQUAL Release" OFF)
    if(QT_ANDROID_SIGN_APK)
        message(STATUS "Signing APK")
        message(STATUS "QT_ANDROID_KEYSTORE_PATH $ENV{QT_ANDROID_KEYSTORE_PATH}")
        message(STATUS "QT_ANDROID_KEYSTORE_ALIAS $ENV{QT_ANDROID_KEYSTORE_ALIAS}")
        # QT_ANDROID_KEYSTORE_STORE_PASS, QT_ANDROID_KEYSTORE_KEY_PASS
    endif()

    if(NOT Qt6LinguistTools_DIR)
        set(Qt6LinguistTools_DIR ${QT_HOST_PATH}/lib/cmake/Qt6LinguistTools)
    endif()
endif()

find_package(Qt6
    REQUIRED
    COMPONENTS
        Bluetooth
        Charts
        Concurrent
        Core
        Core5Compat
        Location
        Multimedia
        Network
        Positioning
        Quick
        QuickControls2
        QuickWidgets
        OpenGL
        Sensors
        Sql
        Svg
        Test
        TextToSpeech
        Widgets
        Xml
    OPTIONAL_COMPONENTS
        LinguistTools
        SerialPort
    HINTS
        ${QT_LIBRARY_HINTS}
)

# Require 6.6.3 because otherwise libQt6QuickControls2Basic.so.6 &
# libQt6QuickControls2BasicStyleImpl.so.6 are missing.
qt_standard_project_setup(REQUIRES 6.6.3)

qt_policy(
    SET QTP0001 NEW
    SET QTP0002 NEW
)

#######################################################
#                QGroundControl Options
#######################################################

option(QGC_STABLE_BUILD "Stable build option" OFF)
if(NOT QGC_STABLE_BUILD)
    add_compile_definitions(DAILY_BUILD)
endif()

cmake_dependent_option(QGC_BUILD_TESTING "Enable testing" ON "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)
if(QGC_BUILD_TESTING)
    add_compile_definitions(UNITTEST_BUILD) # TODO: QGC_UNITTEST_BUILD
endif()

# option(QGC_DISABLE_MAVLINK_INSPECTOR "Disable Mavlink Inspector" OFF) # This removes QtCharts which is GPL licensed

cmake_dependent_option(QGC_DEBUG_QML "Build QGroundControl with QML debugging/profiling support." OFF "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)
if(QGC_DEBUG_QML)
    message(STATUS "To enable the QML debugger/profiler, run with: '-qmljsdebugger=port:1234'")
    add_compile_definitions(QT_QML_DEBUG)
endif()

cmake_dependent_option(QGC_NO_SERIAL_LINK "Build QGroundControl without Serial Support Support." OFF "NOT IOS" ON)

if(QGC_DISABLE_APM_MAVLINK)
    add_compile_definitions(NO_ARDUPILOT_DIALECT)
endif()

#######################################################
#                Custom Build Configuration
#######################################################

if(QGC_CUSTOM_BUILD)
    add_subdirectory(custom)
endif()

if(QGC_DISABLE_APM_MAVLINK)
    add_compile_definitions(NO_ARDUPILOT_DIALECT)
endif()

#######################################################
#                QGroundControl Resources
#######################################################

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

if(CONFIG_UTM_ADAPTER)
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

qt_add_executable(${PROJECT_NAME}
    src/main.cc
    ${QGC_RESOURCES}
)

if(Qt6LinguistTools_FOUND)
    file(GLOB TS_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/translations/qgc_*.ts)
    qt_add_translations(${PROJECT_NAME} TS_FILES ${TS_SOURCES}) # TODO: Update to new qt_add_translations form in Qt6.7
endif()

set_target_properties(${PROJECT_NAME}
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
        "${CMAKE_SOURCE_DIR}/deploy/windows/resource.h"
    )
    # windows installer files different for core and custom
    if(IS_FILE ${CMAKE_SOURCE_DIR}/custom/deploy/windows/installheader.bmp)
        list(APPEND DEPLOY_WIN_FILES ${CMAKE_SOURCE_DIR}/custom/deploy/windows/installheader.bmp)
    else()
        list(APPEND DEPLOY_WIN_FILES ${CMAKE_SOURCE_DIR}/deploy/windows/installheader.bmp)
    endif()
    if(IS_FILE ${CMAKE_SOURCE_DIR}/custom/deploy/windows/WindowsQGC.ico)
        list(APPEND DEPLOY_WIN_FILES ${CMAKE_SOURCE_DIR}/custom/deploy/windows/WindowsQGC.ico)
    else()
        list(APPEND DEPLOY_WIN_FILES ${CMAKE_SOURCE_DIR}/deploy/windows/WindowsQGC.ico)
    endif()

    # Destination directory where files will be copied
    set(DEPLOY_WIN_COPY_DEST "${CMAKE_BINARY_DIR}/deploy/windows")
    file(MAKE_DIRECTORY ${DEPLOY_WIN_COPY_DEST})
    foreach(FILE ${DEPLOY_WIN_FILES})
        # filename without the path
        get_filename_component(FILE_NAME ${FILE} NAME)
        # re-copy the file if it changes
        add_custom_command(
            OUTPUT "${DEPLOY_WIN_COPY_DEST}/${FILE_NAME}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FILE}" "${DEPLOY_WIN_COPY_DEST}/${FILE_NAME}"
            DEPENDS "${FILE}"  # Depend on the source file so that it re-copies when it changes
            COMMENT "Copying ${FILE_NAME} to ${DEPLOY_WIN_COPY_DEST}"
        )
        list(APPEND DEPLOY_WIN_COPIED_FILES "${DEPLOY_WIN_COPY_DEST}/${FILE_NAME}")
    endforeach()

    target_sources(${PROJECT_NAME}
        PRIVATE
            ${DEPLOY_WIN_COPIED_FILES}
    )
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            WIN32_EXECUTABLE TRUE
            QT_TARGET_RC_ICONS "${CMAKE_BINARY_DIR}/deploy/windows/WindowsQGC.ico"
    )
    add_compile_definitions(_USE_MATH_DEFINES)
elseif(MACOS)
    set(MACOSX_BUNDLE_ICON_FILE "macx.icns")
    set(app_icon_macos "${QGC_MACOS_ICON_PATH}/macx.icns")
    set_source_files_properties(${app_icon_macos}
        PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${app_icon_macos})
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/deploy/mac/MacOSXBundleInfo.plist.in"
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
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            QT_IOS_LAUNCH_SCREEN ${CMAKE_SOURCE_DIR}/deploy/ios/QGCLaunchScreen.xib
    )
elseif(ANDROID)
    FetchContent_Declare(android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(${PROJECT_NAME})

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

    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            # QT_ANDROID_ABIS ${ANDROID_ABI}
            QT_ANDROID_MIN_SDK_VERSION 26
            QT_ANDROID_TARGET_SDK_VERSION 34
            QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/android
            QT_ANDROID_VERSION_NAME ${CMAKE_PROJECT_VERSION}
            QT_ANDROID_VERSION_CODE ${ANDROID_VERSION_CODE}
            # QT_QML_ROOT_PATH ${CMAKE_SOURCE_DIR}
    )
    # get_target_property(QGC_ANDROID_DEPLOY_FILE ${PROJECT_NAME} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
elseif(LINUX)

endif()

add_compile_definitions(
    QGC_APP_NAME="${CMAKE_PROJECT_NAME}"
    QGC_ORG_NAME="${QGC_ORG_NAME}"
    QGC_ORG_DOMAIN="${QGC_ORG_DOMAIN}"
    QGC_APP_VERSION_STR="${QGC_APP_VERSION_STR}"
)

add_subdirectory(src)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Widgets
        Qt6::Svg # Used to import QSvgPlugin
        QGC
        QmlControls
        Utilities
)
if(QGC_BUILD_TESTING)
    add_subdirectory(test)
    target_link_libraries(${PROJECT_NAME} PRIVATE qgctest)
endif()

qt_import_plugins(${PROJECT_NAME}
    INCLUDE Qt6::QSvgPlugin
    EXCLUDE_BY_TYPE geoservices
    INCLUDE_BY_TYPE sqldrivers Qt6::QSQLiteDriverPlugin
)

#######################################################
#		Install Configuration
#######################################################

include(InstallRequiredSystemLibraries)

install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    BUNDLE  DESTINATION .
)

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    # DEPLOY_TOOL_OPTIONS
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})

if(LINUX)
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
        FILES ${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png
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
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateAppImage.cmake")
elseif(WIN32)
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateWinInstaller.cmake")
elseif(MACOS)
    install(SCRIPT "${CMAKE_SOURCE_DIR}/cmake/CreateMacDMG.cmake")
endif()
