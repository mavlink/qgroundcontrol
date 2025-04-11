set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/FactSystemTestBase.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FactSystemTestBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FactSystemTestGeneric.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FactSystemTestGeneric.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FactSystemTestPX4.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FactSystemTestPX4.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ParameterManagerTest.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ParameterManagerTest.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# qt_add_qml_module(${CMAKE_PROJECT_NAME}
#     URI factsystemtest
#     VERSION 1.0
#     QML_FILES
#         FactSystemTest.qml
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
# )
