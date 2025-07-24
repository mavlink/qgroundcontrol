target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FactSystemTestBase.cc
        FactSystemTestBase.h
        FactSystemTestGeneric.cc
        FactSystemTestGeneric.h
        FactSystemTestPX4.cc
        FactSystemTestPX4.h
        ParameterManagerTest.cc
        ParameterManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_qml_module(${CMAKE_PROJECT_NAME}
#     URI factsystemtest
#     VERSION 1.0
#     QML_FILES
#         FactSystemTest.qml
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
# )
