message(STATUS "Adding Custom Plugin")
find_package(Qt6 REQUIRED COMPONENTS Core)

#-- Version control
#   Major and minor versions are defined here (manually)
set(CUSTOM_QGC_VER_MAJOR 0)
set(CUSTOM_QGC_VER_MINOR 0)
set(CUSTOM_QGC_VER_FIRST_BUILD 0)

# Build number is automatic
# Uses the current branch. This way it works on any branch including build-server's PR branches
execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir "${CMAKE_SOURCE_DIR}/.git" --work-tree "${CMAKE_SOURCE_DIR}" rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${GIT_EXECUTABLE} --git-dir "${CMAKE_SOURCE_DIR}/.git" rev-list ${GIT_BRANCH} --first-parent --count
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE CUSTOM_QGC_VER_BUILD
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
math(EXPR CUSTOM_QGC_VER_BUILD "${CUSTOM_QGC_VER_BUILD} - ${CUSTOM_QGC_VER_FIRST_BUILD}")
set(CUSTOM_QGC_VERSION "${CUSTOM_QGC_VER_MAJOR}.${CUSTOM_QGC_VER_MINOR}.${CUSTOM_QGC_VER_BUILD}")
message(STATUS "Custom QGC Version: ${CUSTOM_QGC_VERSION}")

# Build a single flight stack by disabling APM support
message(STATUS "Disable APM MAVLink support")
set(QGC_DISABLE_APM_MAVLINK ON PARENT_SCOPE)
message(STATUS "Disable APM Plugin")
set(QGC_DISABLE_APM_PLUGIN ON PARENT_SCOPE)
message(STATUS "Disable APM Plugin Factory")
set(QGC_DISABLE_APM_PLUGIN_FACTORY ON PARENT_SCOPE)
message(STATUS "Disable ArduPilot dialect")
set(QGC_NO_ARDUPILOT_DIALECT ON PARENT_SCOPE)
set_property(DIRECTORY ${CMAKE_SOURCE_DIR}
    APPEND PROPERTY COMPILE_DEFINITIONS
    NO_ARDUPILOT_DIALECT
)

# We implement our own PX4 plugin factory
message(STATUS "Disable PX4 Plugin Factory")
set(QGC_DISABLE_PX4_PLUGIN_FACTORY ON PARENT_SCOPE)

# Enable custom build
set_property(DIRECTORY ${CMAKE_SOURCE_DIR}
    APPEND PROPERTY COMPILE_DEFINITIONS
    QGC_CUSTOM_BUILD
    CUSTOMHEADER="CustomPlugin.h"
    CUSTOMCLASS=CustomPlugin
    QGC_APPLICATION_NAME="Custom QGroundControl"
    QGC_ORG_NAME="Custom"
    QGC_ORG_DOMAIN="org.custom"
    APP_VERSION_STR="${CUSTOM_QGC_VERSION}"
)

# Our own, custom resources
list(APPEND CUSTOM_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/qgcimages.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/qgcresources.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/qgroundcontrol.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/custom.qrc
)
set(QGC_RESOURCES ${CUSTOM_RESOURCES} PARENT_SCOPE)

set(QML_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/res"
    CACHE STRING "Qt Creator 4.1 extra qml import paths" PARENT_SCOPE)


qt_add_library(custom STATIC
    src/CustomPlugin.cc
    src/CustomPlugin.h
    src/AutoPilotPlugin/CustomAutoPilotPlugin.cc
    src/AutoPilotPlugin/CustomAutoPilotPlugin.h
    src/FirmwarePlugin/CustomFirmwarePlugin.cc
    src/FirmwarePlugin/CustomFirmwarePlugin.h
    src/FirmwarePlugin/CustomFirmwarePluginFactory.cc
    src/FirmwarePlugin/CustomFirmwarePluginFactory.h
)

target_link_libraries(custom
    PRIVATE
        FirmwarePlugin
        PX4AutoPilotPlugin
        PX4FirmwarePlugin
        Settings
        Vehicle
    PUBLIC
        Qt6::Core
        QGC
)

target_include_directories(custom
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin
)
