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
