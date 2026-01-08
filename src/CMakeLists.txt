# ============================================================================
# QGroundControl Module Library
# ============================================================================

qt_add_library(QGroundControlModule STATIC)

# ----------------------------------------------------------------------------
# QML Resources
# ----------------------------------------------------------------------------
set_source_files_properties(UI/MainWindow.qml
    PROPERTIES
        QT_RESOURCE_ALIAS MainWindow.qml
)

qt_add_qml_module(QGroundControlModule
    URI QGroundControl
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        UI/MainWindow.qml
    IMPORTS
        QGC
    NO_PLUGIN
)

# ============================================================================
# Subdirectories
# ============================================================================

# Core subsystems
add_subdirectory(ADSB)
add_subdirectory(AnalyzeView)
add_subdirectory(API)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(Camera)
add_subdirectory(Comms)
add_subdirectory(FactSystem)
add_subdirectory(FirmwarePlugin)
add_subdirectory(FlyView)
add_subdirectory(FlightMap)
add_subdirectory(FollowMe)
add_subdirectory(Gimbal)
add_subdirectory(GPS)
add_subdirectory(Joystick)
add_subdirectory(MAVLink)
add_subdirectory(MissionManager)
add_subdirectory(PositionManager)
add_subdirectory(QmlControls)
add_subdirectory(Settings)
add_subdirectory(Terrain)
add_subdirectory(UI)
add_subdirectory(Utilities)
add_subdirectory(UTMSP)
add_subdirectory(Vehicle)
add_subdirectory(VideoManager)
add_subdirectory(Viewer3D)

# Platform-specific
add_subdirectory(Android)

# Qt plugins
add_subdirectory(QtLocationPlugin)

# ============================================================================
# Main Application Sources
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        main.cc
        pch.h
        QGCApplication.cc
        QGCApplication.h
)

# Platform-specific sources
if(NOT ANDROID AND NOT IOS)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            RunGuard.cc
            RunGuard.h
    )
endif()

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
        Qt6::Charts
        Qt6::Concurrent
        Qt6::Core
        Qt6::Core5Compat
        Qt6::CorePrivate
        Qt6::Gui
        Qt6::HttpServer
        Qt6::Network
        Qt6::Sensors
        Qt6::StateMachine
        Qt6::Svg
        Qt6::TextToSpeech
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
        Qt6::PositioningPrivate
        Qt6::Sql

        # Qt6 Multimedia
        Qt6::Multimedia
        Qt6::MultimediaQuickPrivate

        # QGC Modules
        AnalyzeViewModule
        AppSettingsModule
        AutoPilotPluginsCommonModule
        FactControlsModule
        FirstRunPromptDialogsModule
        FlyViewModule
        FlightMapModule
        QGroundControlControlsModule
        QGroundControlModule
        ToolbarModule
        UTMSPModule
        VehicleSetupModule
)

# ----------------------------------------------------------------------------
# Optional/Conditional Modules
# ----------------------------------------------------------------------------
if(NOT QGC_DISABLE_APM_PLUGIN)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE AutoPilotPluginsAPMModule)
endif()

if(NOT QGC_DISABLE_PX4_PLUGIN)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE AutoPilotPluginsPX4Module)
endif()

if(QGC_VIEWER3D)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Viewer3DModule)
endif()

if(NOT QGC_AIRLINK_DISABLED)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE AirLinkModule)
endif()

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
        $<$<NOT:$<BOOL:${QGC_STABLE_BUILD}>>:QGC_DAILY_BUILD>
        $<$<BOOL:${QGC_DISABLE_APM_MAVLINK}>:QGC_NO_ARDUPILOT_DIALECT>
)

# Build configuration-specific definitions
target_compile_definitions(${CMAKE_PROJECT_NAME}
    PRIVATE
        QT_DISABLE_DEPRECATED_UP_TO=${QGC_QT_DISABLE_DEPRECATED_UP_TO}
        QT_ENABLE_STRICT_MODE_UP_TO=${QGC_QT_ENABLE_STRICT_MODE_UP_TO}
)

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
# Precompiled Headers
# ----------------------------------------------------------------------------
target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE pch.h)

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
