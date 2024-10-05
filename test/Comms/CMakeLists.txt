find_package(Qt6 REQUIRED COMPONENTS Core Qml Test)

qt_add_library(CommsTest STATIC
    QGCSerialPortInfoTest.cc
    QGCSerialPortInfoTest.h
)

target_link_libraries(CommsTest
    PRIVATE
        Qt6::Test
        Comms
    PUBLIC
        qgcunittest
)

target_include_directories(CommsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_qml_module(CommsTest
    URI commstest
    VERSION 1.0
    QML_FILES
        MockLinkOptionsDlg.qml
    IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
)
