# ============================================================================
# QGroundControl Custom Plugin Example
# Template for building custom QGroundControl plugins
# ============================================================================

message(STATUS "QGC: Adding Custom Plugin")

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
# set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/OnboardComputers/OnboardComputersInfo.qml PROPERTIES QT_RESOURCE_ALIAS /Custom/src/VehicleSetup/OnboardComputersInfo.qml)
qgc_set_qt_resource_alias(
     # ${CMAKE_CURRENT_SOURCE_DIR}src/OnboardComputers/OnboardComputersInfo.qml
)

qt_add_qml_module(CustomModule
    URI Custom.Widgets
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
#         ${CMAKE_CURRENT_SOURCE_DIR}/src/OnboardComputers/OnboardComputersInfo.qml
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

message(STATUS "QGC_CUSTOM_VERSION: ${QGC_CUSTOM_VERSION}")

# Adding custom version for QGC
set(QGC_CUSTOM_VERSION "1.0.1" CACHE STRING "Custom version of QGC")
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
