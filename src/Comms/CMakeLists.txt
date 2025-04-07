add_subdirectory(AirLink)
add_subdirectory(MockLink)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        LinkConfiguration.cc
        LinkConfiguration.h
        LinkInterface.cc
        LinkInterface.h
        LinkManager.cc
        LinkManager.h
        LogReplayLink.cc
        LogReplayLink.h
        LogReplayLinkController.cc
        LogReplayLinkController.h
        MAVLinkProtocol.cc
        MAVLinkProtocol.h
        TCPLink.cc
        TCPLink.h
        UDPLink.cc
        UDPLink.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

if(QGC_NO_SERIAL_LINK)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_NO_SERIAL_LINK)
else()
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            QGCSerialPortInfo.cc
            QGCSerialPortInfo.h
            SerialLink.cc
            SerialLink.h
            UdpIODevice.cc
            UdpIODevice.h
    )

    if(ANDROID)
        add_subdirectory(${CMAKE_SOURCE_DIR}/android/libs/qtandroidserialport qtandroidserialport.build)
        target_sources(${CMAKE_PROJECT_NAME}
            PRIVATE
                ${CMAKE_SOURCE_DIR}/android/src/AndroidSerial.cc
                ${CMAKE_SOURCE_DIR}/android/src/AndroidSerial.h
        )
        target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/android/src)
    else()
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::SerialPort)
    endif()
endif()

#===========================================================================#

if(QGC_ENABLE_BLUETOOTH)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Bluetooth)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            BluetoothLink.cc
            BluetoothLink.h
    )
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_ENABLE_BLUETOOTH)
endif()

#===========================================================================#

if(QGC_ZEROCONF_ENABLED)
    message(STATUS "Building QMDNSEngine")

    CPMAddPackage(
        NAME qmdnsengine
        GITHUB_REPOSITORY nitroshare/qmdnsengine
        GIT_TAG master
        OPTIONS
            "BUILD_DOC OFF"
            "BUILD_EXAMPLES OFF"
            "BUILD_TESTS OFF"
    )

    if(TARGET qmdnsengine)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE qmdnsengine)
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_ZEROCONF_ENABLED)
    endif()
endif()

# target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QABSTRACTSOCKET_DEBUG)
