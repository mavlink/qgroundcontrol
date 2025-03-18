message(STATUS "Adding Custom Plugin")
find_package(Qt6 REQUIRED COMPONENTS Core)

# Enable custom build
set_property(DIRECTORY ${CMAKE_SOURCE_DIR}
    APPEND PROPERTY COMPILE_DEFINITIONS
    QGC_CUSTOM_BUILD
    CUSTOMHEADER="CustomPlugin.h"
    CUSTOMCLASS=CustomPlugin
)

if(ANDROID)
    set(CUSTOM_ANDROID_DIR "${CMAKE_SOURCE_DIR}/custom/android")
    if(EXISTS "${CUSTOM_ANDROID_DIR}")
        file(GLOB CUSTOM_ANDROID_FILES "${CUSTOM_ANDROID_DIR}/*")
        if(CUSTOM_ANDROID_FILES)
            message(STATUS "Custom Android package template found. Overlaying custom files...")
            set(DEFAULT_ANDROID_DIR "${CMAKE_SOURCE_DIR}/android")
            set(FINAL_ANDROID_DIR "${CMAKE_BINARY_DIR}/custom/android")
            file(COPY "${DEFAULT_ANDROID_DIR}/." DESTINATION "${FINAL_ANDROID_DIR}")
            file(COPY "${CUSTOM_ANDROID_DIR}/." DESTINATION "${FINAL_ANDROID_DIR}")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
                            "${DEFAULT_ANDROID_DIR}/"
                            "${CUSTOM_ANDROID_DIR}/"
                        )
            set(QGC_ANDROID_PACKAGE_SOURCE_DIR "${FINAL_ANDROID_DIR}" CACHE PATH "Path to a custom Android package template" FORCE)
            message(STATUS "Android package template path will be set to: ${QGC_ANDROID_PACKAGE_SOURCE_DIR}")
        else()
            message(STATUS "Custom Android package template empty. Using default.")
        endif()
    else()
        message(STATUS "No custom Android package template found. Using default.")
    endif()
endif()

find_package(Python3)
if(Python3_FOUND)
    execute_process(
        COMMAND ${Python3_EXECUTABLE}
        RESULT_VARIABLE PYTHON_RESULT
        OUTPUT_VARIABLE PYTHON_OUTPUT
        ERROR_VARIABLE PYTHON_ERROR
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        INPUT_FILE updateqrc.py
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    cmake_print_variables(PYTHON_RESULT PYTHON_OUTPUT PYTHON_ERROR)
endif()

# Our own, custom resources
list(APPEND CUSTOM_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/qgcimages.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/qgcresources.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/qgroundcontrol.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/custom.qrc
)
set(QGC_RESOURCES ${QGC_RESOURCES} ${CUSTOM_RESOURCES} CACHE STRING "Paths to .qrc Resources" FORCE)

set(QML_IMPORT_PATH ${QML_IMPORT_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/res" CACHE STRING "Extra qml import paths" FORCE)

qt_add_library(CustomPlugin STATIC
    src/CustomPlugin.cc
    src/CustomPlugin.h
    src/AutoPilotPlugin/CustomAutoPilotPlugin.cc
    src/AutoPilotPlugin/CustomAutoPilotPlugin.h
    src/FirmwarePlugin/CustomFirmwarePlugin.cc
    src/FirmwarePlugin/CustomFirmwarePlugin.h
    src/FirmwarePlugin/CustomFirmwarePluginFactory.cc
    src/FirmwarePlugin/CustomFirmwarePluginFactory.h
)

target_link_libraries(CustomPlugin
    PUBLIC
        Qt6::Core
        QGC
)

target_include_directories(CustomPlugin
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin
)

# file(GLOB_RECURSE QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/res/*.qml)
# file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/res/Images/*.*)
# qt_add_qml_module(CustomPlugin
#     URI QGroundControl.CustomPlugin
#     VERSION 1.0
#     RESOURCE_PREFIX "/"
#     QML_FILES ${QML_SOURCES}
#     RESOURCES ${QML_RESOURCES}
#     OUTPUT_TARGETS CustomPlugin_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Layouts
#         QGroundControl
#         QGroundControl.Controls
#         QGroundControl.Palette
#         QGroundControl.ScreenTools
# )
