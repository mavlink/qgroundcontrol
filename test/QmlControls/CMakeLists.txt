# ============================================================================
# QmlControls Unit Tests
# Tests for QML control models and utilities
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCGeoBoundingCubeTest.cc
        QGCGeoBoundingCubeTest.h
        QmlObjectListModelTest.cc
        QmlObjectListModelTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
