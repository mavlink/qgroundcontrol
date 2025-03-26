find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick Test)

target_sources(qgctest
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

target_link_libraries(qgctest
    PRIVATE
        Qt6::Quick
        Qt6::Test
)

target_include_directories(qgctest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_qml_module(qgctest
#     URI factsystemtest
#     VERSION 1.0
#     QML_FILES
#         FactSystemTest.qml
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
# )
