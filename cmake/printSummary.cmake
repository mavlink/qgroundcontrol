# output chosen build options
macro( OptionOutput _outputstring )
    if( ${ARGN} )
        set( _var "YES" )
    else( ${ARGN} )
        set( _var "NO" )
    endif( ${ARGN} )
    message( STATUS "${_outputstring}${_var}" )
endmacro( OptionOutput _outputstring )

include(CMakePrintHelpers)
# cmake_print_properties
# cmake_host_system_information

message( STATUS "------------------------------------------------------------------" )
message( STATUS "" )
include(CMakePrintSystemInformation)
message( STATUS "" )
message( STATUS "------------------------------------------------------------------" )
message( STATUS "" )
message( STATUS "CMAKE_INSTALL_PREFIX:        ${CMAKE_INSTALL_PREFIX}" )
message( STATUS "CMAKE_GENERATOR:             ${CMAKE_GENERATOR}" )
message( STATUS "CMAKE_BUILD_TYPE:            ${CMAKE_BUILD_TYPE}" )
message( STATUS "CMAKE_CXX_COMPILER:          ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER}) ; version: ${CMAKE_CXX_COMPILER_VERSION}" )
message( STATUS "CMAKE_CXX_STANDARD:          ${CMAKE_CXX_STANDARD}" )
message( STATUS "CMAKE_CXX_COMPILER_LAUNCHER: ${CMAKE_CXX_COMPILER_LAUNCHER}" )
message( STATUS "CMAKE_CXX_COMPILER_LAUNCHER: ${CMAKE_CXX_COMPILER_LAUNCHER}" )
# CMAKE_CXX_EXTENSIONS
message( STATUS "CMAKE_C_COMPILER_LAUNCHER:   ${CMAKE_C_COMPILER_LAUNCHER}" )
if(APPLE)
message( STATUS "CMAKE_OBJC_COMPILER_LAUNCHER:${CMAKE_OBJC_COMPILER_LAUNCHER}" )
# CMAKE_OSX_SYSROOT
# CMAKE_OSX_ARCHITECTURES
# CMAKE_OSX_DEPLOYMENT_TARGET
endif()
# if(MSVC)
# CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
# endif()
# if(WIN32)
# CMAKE_WIN32_EXECUTABLE
# endif()
# CMAKE_PROJECT_NAME
message( STATUS "CMAKE_VERSION:               ${CMAKE_VERSION}" )
message( STATUS "CMAKE_PREFIX_PATH:           ${CMAKE_PREFIX_PATH}" )
message( STATUS "CMAKE_HOST_SYSTEM_NAME:      ${CMAKE_HOST_SYSTEM_NAME}" )
message( STATUS "CMAKE_HOST_SYSTEM_VERSION:   ${CMAKE_HOST_SYSTEM_VERSION}" )
message( STATUS "CMAKE_SYSTEM_NAME:           ${CMAKE_SYSTEM_NAME}" )
message( STATUS "CMAKE_SYSTEM_VERSION:        ${CMAKE_SYSTEM_VERSION}" )
message( STATUS "CMAKE_SOURCE_DIR:            ${CMAKE_SOURCE_DIR}" )
message( STATUS "CMAKE_BINARY_DIR:            ${CMAKE_BINARY_DIR}" )
message( STATUS "CMAKE_TOOLCHAIN_FILE:        ${CMAKE_TOOLCHAIN_FILE}" )
message( STATUS "CMAKE_CROSSCOMPILING         ${CMAKE_CROSSCOMPILING}" )
CMAKE_AUTOMOC
CMAKE_AUTOUIC
CMAKE_AUTORCC
# CMAKE_POSITION_INDEPENDENT_CODE
# CMAKE_INTERPROCEDURAL_OPTIMIZATION
# CMAKE_AUTOGEN_BETTER_GRAPH_MULTI_CONFIG
# CMAKE_EXPORT_COMPILE_COMMANDS
# if(CMAKE_CROSSCOMPILING)
# CMAKE_HOST_SYSTEM_PROCESSOR
# CMAKE_SYSTEM_PROCESSOR
# endif()
message( STATUS "" )
message( STATUS " --- Compiler flags --- ")
message( STATUS "General:                     ${CMAKE_CXX_FLAGS}" )
message( STATUS "Extra:                       ${EXTRA_COMPILE_FLAGS}" )
message( STATUS "Debug:                       ${CMAKE_CXX_FLAGS_DEBUG}" )
message( STATUS "Release:                     ${CMAKE_CXX_FLAGS_RELEASE}" )
message( STATUS "RelWithDebInfo:              ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" )
message( STATUS "MinSizeRel:                  ${CMAKE_CXX_FLAGS_MINSIZEREL}" )
message( STATUS "" )
message( STATUS " --- Linker flags --- ")
message( STATUS "General:                     ${CMAKE_EXE_LINKER_FLAGS}" )
message( STATUS "Debug:                       ${CMAKE_EXE_LINKER_FLAGS_DEBUG}" )
message( STATUS "Release:                     ${CMAKE_EXE_LINKER_FLAGS_RELEASE}" )
message( STATUS "RelWithDebInfo:              ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}" )
message( STATUS "MinSizeRel:                  ${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}" )
message( STATUS "" )
message( STATUS "------------------------------------------------------------------" )
message( STATUS "" )
message( STATUS "App Name:                    ${QGC_APP_NAME}" )
message( STATUS "App Copyright:               ${QGC_APP_COPYRIGHT}" )
message( STATUS "App Description:             ${QGC_APP_DESCRIPTION}" )
message( STATUS "Org Name:                    ${QGC_ORG_NAME}" )
message( STATUS "App Domain:                  ${QGC_ORG_DOMAIN}" )
message( STATUS "App Version:                 ${QGC_APP_VERSION}" )
# QGC_APP_TAG
# QGC_SETTINGS_VERSION
if(MACOS)
message( STATUS "MacOS Bundle ID:             ${QGC_BUNDLE_ID}" )
message( STATUS "MacOS Icon Path:             ${QGC_MACOS_ICON_PATH}" )
# QGC_MACOS_UNIVERSAL_BUILD
endif()
if(WIN32)
QGC_INSTALLER_SOURCE_WIN
endif()

# QGC_CUSTOM_BUILD
OptionOutput( "Stable Build:                " QGC_STABLE_BUILD )
OptionOutput( "Building Tests:              " QGC_BUILD_TESTING AND BUILD_TESTING )
OptionOutput( "Build Dependencies:          " QGC_BUILD_DEPENDENCIES )
OptionOutput( "Disable APM Dialect:         " QGC_DISABLE_APM_MAVLINK )
OptionOutput( "Disable APM Plugin:          " QGC_DISABLE_APM_PLUGIN )
OptionOutput( "Disable APM Plugin Factory:  " QGC_DISABLE_APM_PLUGIN_FACTORY )
OptionOutput( "Disable PX4 Plugin:          " QGC_DISABLE_PX4_PLUGIN )
OptionOutput( "Disable PX4 Plugin Factory:  " QGC_DISABLE_PX4_PLUGIN_FACTORY )
OptionOutput( "Enable Bluetooth Links:      " QGC_ENABLE_BLUETOOTH )
OptionOutput( "Enable ZeroConf:             " QGC_ZEROCONF_ENABLED )
OptionOutput( "Disable AIRLink:             " QGC_AIRLINK_DISABLED )
OptionOutput( "Disable Serial Links:        " QGC_NO_SERIAL_LINK )
OptionOutput( "Enable UTM Adapter:          " QGC_UTM_ADAPTER )
OptionOutput( "Enable Viewer3D:             " QGC_VIEWER3D )
OptionOutput( "Enable UVC Devices:          " QGC_ENABLE_UVC )
OptionOutput( "Enable GStreamer:            " QGC_ENABLE_GST_VIDEOSTREAMING )
OptionOutput( "Enable QtMultimedia:         " QGC_ENABLE_QT_VIDEOSTREAMING )
message( STATUS "MAVLink Git Repo:            ${QGC_MAVLINK_GIT_REPO}" )
message( STATUS "MAVLink Git Tag:             ${QGC_MAVLINK_GIT_TAG}" )
# QGC_RESOURCES
message( STATUS "" )
message( STATUS "------------------------------------------------------------------" )
message( STATUS "" )

OptionOutput( "Debug QML:                   " QT_QML_DEBUG )
# NDEBUG
# QT_NO_DEBUG
# QT_NO_DEBUG_OUTPUT
# QT_FATAL_WARNINGS
# QT_NO_DEBUG_OUTPUT
# QT_NO_INFO_OUTPUT
# QT_NO_WARNING_OUTPUT

# QT_QML_OUTPUT_DIRECTORY

# QT_I18N_SOURCE_LANGUAGE
# QT_I18N_TRANSLATED_LANGUAGES

# if(IOS)
# QT_IOS_LAUNCH_SCREEN
# endif()

if(ANDROID)
message( STATUS "Android NDK Host System      ${ANDROID_NDK_HOST_SYSTEM_NAME}" )
message( STATUS "Android SDK Root             ${ANDROID_SDK_ROOT}" )
# message(STATUS "QT_ANDROID_KEYSTORE_PATH $ENV{QT_ANDROID_KEYSTORE_PATH}")
# message(STATUS "QT_ANDROID_KEYSTORE_ALIAS $ENV{QT_ANDROID_KEYSTORE_ALIAS}")
# message(STATUS "QT_ANDROID_KEYSTORE_STORE_PASS $ENV{QT_ANDROID_KEYSTORE_STORE_PASS}")
# message(STATUS "QT_ANDROID_KEYSTORE_KEY_PASS $ENV{QT_ANDROID_KEYSTORE_KEY_PASS}")
# QT_ANDROID_MULTI_ABI_FORWARD_VARS
# QT_ANDROID_DEPLOYMENT_TYPE
# QT_ANDROID_GENERATE_JAVA_QTQUICKVIEW_CONTENTS
# QT_ANDROID_DEPLOYMENT_TYPE
# QT_ANDROID_SIGN_APK
# QT_ANDROID_SIGN_AAB
# QT_USE_TARGET_ANDROID_BUILD_DIR
# QT_ANDROID_ABIS
# QT_ANDROID_BUILD_ALL_ABIS # Overrides QT_ANDROID_ABIS
# QT_PATH_ANDROID_ABI_armeabi-v7a
# QT_PATH_ANDROID_ABI_arm64-v8a
# QT_PATH_ANDROID_ABI_x86
# QT_PATH_ANDROID_ABI_x86_64
endif()

# QT_MINIMUM_VERSION
# QT_MAXIMUM_VERSION
# QT_DEPLOY_BIN_DIR
# QT_DEPLOY_IGNORED_LIB_DIRS
# QT_DEPLOY_LIBEXEC_DIR
# QT_DEPLOY_LIB_DIR
# QT_DEPLOY_PLUGINS_DIR
# QT_DEPLOY_PREFIX
# QT_DEPLOY_QML_DIR
# QT_DEPLOY_SUPPORT
# QT_DEPLOY_TRANSLATIONS_DIR
# QT_ENABLE_VERBOSE_DEPLOYMENT

# QT_HOST_PATH

# deploy_script
# deploy_tool_options_arg

# Environment Variables
# SET(ENV{QT_MESSAGE_PATTERN} "[%{time process} %{type}] %{appname} %{category} %{function} - %{message}")
# set(ENV{QT_DEBUG_PLUGINS} ON)
# set(ENV{QML_IMPORT_TRACE} ON)
# SET(ENV{QT_HASH_SEED} 1234)
# SET(ENV{QT_WIN_DEBUG_CONSOLE} new)

# Project Target Properties
# get_target_property(QGC_ANDROID_DEPLOY_FILE ${CMAKE_PROJECT_NAME} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
