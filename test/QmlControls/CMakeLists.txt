# ============================================================================
# QmlControls Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ObjectItemModelBaseTest.cc
        ObjectItemModelBaseTest.h
        ObjectListModelBaseTest.cc
        ObjectListModelBaseTest.h
        QmlObjectListModelTest.cc
        QmlObjectListModelTest.h
        QmlObjectTreeModelTest.cc
        QmlObjectTreeModelTest.h
        TestDirtyObject.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
