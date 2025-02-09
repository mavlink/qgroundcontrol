find_package(Qt6 REQUIRED COMPONENTS Core Qml Test)

qt_add_library(CommsTest STATIC)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(CommsTest
        PRIVATE
            QGCSerialPortInfoTest.cc
            QGCSerialPortInfoTest.h
    )
endif()

target_link_libraries(CommsTest
    PRIVATE
        Qt6::Test
        Comms
    PUBLIC
        qgcunittest
)

target_include_directories(CommsTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
