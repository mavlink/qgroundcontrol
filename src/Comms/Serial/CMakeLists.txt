# Keep the resource available in builds without serial links as well.
qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_comm
    PREFIX "/json"
    FILES USBBoardInfo.json
)

if(QGC_NO_SERIAL_LINK)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_NO_SERIAL_LINK)
    return()
endif()

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE QGCSerialPortInfo.cc
            QGCSerialPortInfo.h
            SerialAutoConnect.cc
            SerialAutoConnect.h
            SerialLink.cc
            SerialLink.h
            SerialPortManager.cc
            SerialPortManager.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT ANDROID)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::SerialPort)
endif()
