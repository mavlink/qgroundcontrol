# ============================================================================
# QML UI Smoke Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/QmlUITestBase.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/QmlUITestBase.h
        ${CMAKE_CURRENT_SOURCE_DIR}/TopLevelViewsTest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/TopLevelViewsTest.h
        ${CMAKE_CURRENT_SOURCE_DIR}/VehicleConfigUITest.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/VehicleConfigUITest.h
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
