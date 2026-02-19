# ============================================================================
# QmlControls Unit Tests
# Tests for QGroundControlQmlGlobal, ScreenToolsController, and related QML controls
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGroundControlQmlGlobalTest.cc QGroundControlQmlGlobalTest.h
        ScreenToolsControllerTest.cc ScreenToolsControllerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
