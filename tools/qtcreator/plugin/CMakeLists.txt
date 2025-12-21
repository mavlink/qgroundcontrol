cmake_minimum_required(VERSION 3.16)

project(QGCPlugin
    VERSION 0.1.0
    DESCRIPTION "QGroundControl Development Tools for Qt Creator"
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt Creator
# Usage: cmake -DCMAKE_PREFIX_PATH="/path/to/qtcreator;/path/to/Qt/6.x/gcc_64" ..
find_package(QtCreator REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

# Extract Qt Creator version for compatibility checks
if(IDE_VERSION)
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${IDE_VERSION})
    set(IDE_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(IDE_VERSION_MINOR ${CMAKE_MATCH_2})
    set(IDE_VERSION_PATCH ${CMAKE_MATCH_3})
    message(STATUS "Building for Qt Creator ${IDE_VERSION}")
endif()

# Plugin sources
set(PLUGIN_SOURCES
    # Core plugin
    src/qgcplugin.h
    src/qgcplugin.cpp
    src/qgcconstants.h
    src/qgcplugin_global.h
    # FactGroup Wizard
    src/wizard/factgroupwizardfactory.h
    src/wizard/factgroupwizardfactory.cpp
    src/wizard/factgroupwizardpage.h
    src/wizard/factgroupwizardpage.cpp
    # MAVLink Completion
    src/completion/mavlinkcompletionassist.h
    src/completion/mavlinkcompletionassist.cpp
    # Vehicle Null-Check Analysis (future)
    # src/analysis/vehiclenullchecker.h
    # src/analysis/vehiclenullchecker.cpp
)

# Add the Qt Creator plugin
add_qtc_plugin(QGCPlugin
    PLUGIN_CLASS QGCPlugin
    PLUGIN_DEPENDS
        QtCreator::Core
        QtCreator::ProjectExplorer
        QtCreator::TextEditor
    DEPENDS
        Qt::Widgets
        Qt::Core
        Qt::Gui
        QtCreator::ExtensionSystem
        QtCreator::Utils
    SOURCES
        ${PLUGIN_SOURCES}
)

# Target to run Qt Creator with the plugin for testing
add_custom_target(RunQtCreator
    COMMAND ${CMAKE_COMMAND} -E env
        "QT_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/qtcreator/plugins"
        ${IDE_BIN_PATH}/qtcreator
    DEPENDS QGCPlugin
    COMMENT "Running Qt Creator with QGCPlugin"
)
