# ============================================================================
# Serial Communication
# QSerialPort-backed link on host (HostSerialPort); on Android, makeSerialPort() routes USB-host
# locations to JNI-backed AndroidSerialPort and "/dev/tty*" paths to HostSerialPort/QSerialPort
# (used by devices like the Radiomaster AX12 that expose firmware UARTs via kernel TTYs).
# ============================================================================

if(QGC_NO_SERIAL_LINK)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_NO_SERIAL_LINK)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        HostSerialPort.cc
        HostSerialPort.h
        NmeaSerialDevice.cc
        NmeaSerialDevice.h
        QGCSerialPort.h
        QGCSerialPortInfo.cc
        QGCSerialPortInfo.h
        QGCSerialPortTypes.h
        SerialPlatform.cc
        SerialPlatform.h
        SerialLink.cc
        SerialLink.h
)

if(ANDROID)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE QGCSerialPortInfo_android.cc)
else()
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE QGCSerialPortInfo_host.cc)
endif()

qt_add_resources(${CMAKE_PROJECT_NAME} json_serial
    PREFIX "/json"
    FILES USBBoardInfo.json
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::SerialPort)
