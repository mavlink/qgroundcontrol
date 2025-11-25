# ============================================================================
# QGroundControl Module Library
# ============================================================================

qt_add_library(QGroundControlModule STATIC)

qt_add_qml_module(QGroundControlModule
    URI QGroundControl
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        MainWindow.qml
    IMPORTS
        QGC
    NO_PLUGIN
)

# ============================================================================
# Subdirectories - Minimal Map Only
# ============================================================================

# Core subsystems for map
add_subdirectory(FactSystem)
add_subdirectory(FlightMap)
add_subdirectory(QmlControls)
add_subdirectory(Settings)
add_subdirectory(Terrain)
add_subdirectory(Utilities)

# Qt plugins
add_subdirectory(QtLocationPlugin)

# ============================================================================
# Main Application Sources
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        main.cc
)

# ----------------------------------------------------------------------------
# Include Directories
# ----------------------------------------------------------------------------
target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# Link Libraries
# ============================================================================

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        # Qt6 Core
        Qt6::Concurrent
        Qt6::Core
        Qt6::Core5Compat
        Qt6::Gui
        Qt6::Network
        Qt6::Svg
        Qt6::Widgets
        Qt6::Xml

        # Qt6 QML/Quick
        Qt6::Qml
        Qt6::QmlIntegration
        Qt6::Quick
        Qt6::QuickControls2

        # Qt6 Location
        Qt6::Location
        Qt6::LocationPrivate
        Qt6::Positioning
        Qt6::Sql

        # QGC Modules - Minimal for Map
        FlightMapModule
        QGroundControlModule
)

# ============================================================================
# Compile Definitions
# ============================================================================

# Application metadata
target_compile_definitions(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGC_APP_NAME="${QGC_APP_NAME}"
        QGC_ORG_NAME="${QGC_ORG_NAME}"
        QGC_ORG_DOMAIN="${QGC_ORG_DOMAIN}"
        QGC_APP_VERSION_STR="${QGC_APP_VERSION_STR}"
        QGC_APP_DATE="${QGC_APP_DATE}"
        QGC_APP_DESCRIPTION="${QGC_APP_DESCRIPTION}"
        QGC_SETTINGS_VERSION=${QGC_SETTINGS_VERSION}
)

# Build configuration-specific definitions
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${CMAKE_PROJECT_NAME}
        PRIVATE
            NDEBUG
            QT_NO_DEBUG
            QT_MESSAGELOGCONTEXT
    )
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME}
        PRIVATE
            QT_DISABLE_DEPRECATED_UP_TO=0x060800
            QT_ENABLE_STRICT_MODE_UP_TO=0x060800
            $<$<BOOL:${QGC_DEBUG_QML}>:QT_QML_DEBUG>
    )
endif()

# ============================================================================
# Code Coverage Configuration
# ============================================================================

include(Coverage)

# ============================================================================
# Target Properties
# ============================================================================

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        QT_RESOURCE_PREFIX "/qml"
        OUTPUT_NAME ${CMAKE_PROJECT_NAME}
)

# ----------------------------------------------------------------------------
# Precompiled Headers (Disabled for minimal build)
# ----------------------------------------------------------------------------
# target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE pch.h)

# ============================================================================
# Custom Build Configuration
# ============================================================================

if(QGC_CUSTOM_BUILD)
    if(CUSTOM_QT_COMPONENTS)
        find_package(Qt6 REQUIRED COMPONENTS ${CUSTOM_QT_COMPONENTS})
    endif()

    if(CUSTOM_SOURCES)
        target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${CUSTOM_SOURCES})
    endif()

    if(CUSTOM_LIBRARIES)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CUSTOM_LIBRARIES})
    endif()

    if(CUSTOM_INCLUDE_DIRECTORIES)
        target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CUSTOM_INCLUDE_DIRECTORIES})
    endif()

    if(CUSTOM_DEFINITIONS)
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ${CUSTOM_DEFINITIONS})
    endif()

    message(STATUS "QGC: Custom build configuration applied")
endif()
