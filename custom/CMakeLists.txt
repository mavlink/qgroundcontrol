# ============================================================================
# QGroundControl Custom Plugin Example
# Template for building custom QGroundControl plugins
# ============================================================================

message(STATUS "QGC: Adding Custom Plugin")

# Adding custom version for QGC
set(QGC_CUSTOM_VERSION "1.0.2" CACHE STRING "Custom version of QGC")

# ============================================================================
# Custom Android Package Template
# ============================================================================

if(ANDROID)
    set(CUSTOM_ANDROID_DIR "${CMAKE_SOURCE_DIR}/custom/android")
    if(EXISTS "${CUSTOM_ANDROID_DIR}")
        file(GLOB CUSTOM_ANDROID_FILES "${CUSTOM_ANDROID_DIR}/*")
        if(CUSTOM_ANDROID_FILES)
            message(STATUS "QGC: Custom Android package template found. Overlaying custom files...")
            set(DEFAULT_ANDROID_DIR "${CMAKE_SOURCE_DIR}/android")
            set(FINAL_ANDROID_DIR "${CMAKE_BINARY_DIR}/custom/android")

            # Copy default Android template
            file(COPY "${DEFAULT_ANDROID_DIR}/." DESTINATION "${FINAL_ANDROID_DIR}")

            # Overlay custom Android files
            file(COPY "${CUSTOM_ANDROID_DIR}/." DESTINATION "${FINAL_ANDROID_DIR}")

            # Track changes to Android directories
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                "${DEFAULT_ANDROID_DIR}/"
                "${CUSTOM_ANDROID_DIR}/"
            )

            set(QGC_ANDROID_PACKAGE_SOURCE_DIR "${FINAL_ANDROID_DIR}" CACHE PATH "Path to a custom Android package template" FORCE)
            message(STATUS "QGC: Android package template path will be set to: ${QGC_ANDROID_PACKAGE_SOURCE_DIR}")
        else()
            message(STATUS "QGC: Custom Android package template empty. Using default.")
        endif()
    else()
        message(STATUS "QGC: No custom Android package template found. Using default.")
    endif()
endif()

# ============================================================================
# Custom Resources and Import Paths
# ============================================================================
# Add custom Qt resources
list(APPEND CUSTOM_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/custom.qrc
)
set(QGC_RESOURCES ${QGC_RESOURCES} ${CUSTOM_RESOURCES} CACHE STRING "Paths to .qrc Resources" FORCE)

# Add custom QML import path
set(QML_IMPORT_PATH ${QML_IMPORT_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/res" CACHE STRING "Extra qml import paths" FORCE)

# ============================================================================
# Custom QML Module
# ============================================================================

qt_add_library(CustomModule STATIC)

# Set resource aliases for custom widgets
# deprecated method of setting aliases
# set_source_files_properties( PROPERTIES QT_RESOURCE_ALIAS CustomSimpleMessageDialog.qml)

# new method of setting aliases
qgc_set_qt_resource_alias(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Qml/CustomSimpleMessageDialog.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Qml/CustomPopupDialog.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Toolbar/VioIndicatorPage.qml
)

qt_add_qml_module(CustomModule
    URI Custom.Widgets
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Qml/CustomSimpleMessageDialog.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Qml/CustomPopupDialog.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/res/Toolbar/VioIndicatorPage.qml
    NO_PLUGIN
    SOURCES
)

# ============================================================================
# Custom Plugin Sources
# ============================================================================

set(CUSTOM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FoxFourPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FoxFourPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin/FoxFourAutoPilotPlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin/FoxFourAutoPilotPlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/FoxFourFirmwarePlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/FoxFourFirmwarePlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/FoxFourFirmwarePluginFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/FoxFourFirmwarePluginFactory.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Camera/FoxFourCameraControl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Camera/FoxFourCameraControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Camera/FoxFourCameraManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Camera/FoxFourCameraManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ParameterSetter/ParameterSetter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ParameterSetter/ParameterSetter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OnboardComputers/OnboardComputersManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OnboardComputers/OnboardComputersManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VioGpsComparer/VioGpsComparer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VioGpsComparer/VioGpsComparer.h
    CACHE INTERNAL "adding custom sources" FORCE
)

if(${QGC_ENABLE_GST_VIDEOSTREAMING})
    set(CUSTOM_SOURCES
        ${CUSTOM_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VideoReceiver/FoxFourGstVideoReceiver.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VideoReceiver/FoxFourGstVideoReceiver.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VideoReceiver/KLVMetadata.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/VideoReceiver/KLVMetadata.cpp
        CACHE INTERNAL "adding custom gstreamer" FORCE
    )
endif()
# ============================================================================
# Custom Build Configuration
# ============================================================================

# Custom libraries to link
set(CUSTOM_LIBRARIES
    CustomModule
    CACHE INTERNAL "" FORCE
)

# Custom include directories
set(CUSTOM_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OnboardComputers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ParameterSetter
    CACHE INTERNAL "" FORCE
)

# ============================================================================
# Getting version from git tag if available
# ============================================================================
if(GIT_FOUND)
    # Get the current Git tag
    execute_process(
        COMMAND ${GIT_EXECUTABLE} tag --sort=-version:refname
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_ALL_TAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Extract the first (latest) tag
    string(REGEX MATCH "^[^\n]+" GIT_TAG "${GIT_ALL_TAGS}")

    message(STATUS "GIT TAG IS: ${GIT_TAG}")

    # Get the full describe output (tag + commits + hash)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # Get current commit hash
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_TAG "unknown")
    set(GIT_DESCRIBE "unknown")
    set(GIT_COMMIT_HASH "unknown")
endif()

# Extract version from tag (assumes format like v1.2.3)
if(GIT_TAG MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(VERSION_MAJOR ${CMAKE_MATCH_1})
    set(VERSION_MINOR ${CMAKE_MATCH_2})
    set(VERSION_PATCH ${CMAKE_MATCH_3})
    message(STATUS "Setting QGC_CUSTOM_VERSION TO: ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
    set(QGC_CUSTOM_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" CACHE STRING "Custom version for QGC set to git tag" FORCE)
else()
    message(STATUS "Setting QGC_CUSTOM_VERSION TO: 0.0.0")
    set(QGC_CUSTOM_VERSION "0.0.0" CACHE STRING "Custom version for QGC set to default" FORCE)
endif()

message(STATUS "QGC_CUSTOM_VERSION: ${QGC_CUSTOM_VERSION}")
# Adding custom version for QGC
# set(QGC_CUSTOM_VERSION "1.0.2" CACHE STRING "Custom version of QGC")

# Custom compiler definitions
set(CUSTOM_DEFINITIONS
    QGC_CUSTOM_BUILD
    QGC_CUSTOM_VERSION="${QGC_CUSTOM_VERSION}"
    CUSTOMHEADER="FoxFourPlugin.h"
    CUSTOMCLASS=FoxFourPlugin
    CACHE INTERNAL "setting custom definitions" FORCE
)
message(STATUS "CUSTOM_DEFINITIONS: ${CUSTOM_DEFINITIONS}")
# Custom Qt components required
set(CUSTOM_QT_COMPONENTS
    Core
    CACHE INTERNAL "adding custom components to qgc" FORCE
)
