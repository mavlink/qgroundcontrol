add_subdirectory(AirLink)
add_subdirectory(MockLink)

find_package(Qt6 REQUIRED COMPONENTS Core Network Qml)

target_sources(QGC
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

target_link_libraries(QGC
    PRIVATE
        Qt6::Qml
    PUBLIC
        Qt6::Core
        Qt6::Network
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

if(QGC_NO_SERIAL_LINK)
    target_compile_definitions(QGC PUBLIC QGC_NO_SERIAL_LINK)
else()
    target_sources(QGC
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
        target_sources(qtandroidserialport
            PRIVATE
                ${CMAKE_SOURCE_DIR}/android/src/AndroidSerial.cc
                ${CMAKE_SOURCE_DIR}/android/src/AndroidSerial.h
        )
        target_include_directories(qtandroidserialport PUBLIC ${CMAKE_SOURCE_DIR}/android/src)
        target_link_libraries(QGC PUBLIC qtandroidserialport)
    else()
        find_package(Qt6 REQUIRED COMPONENTS SerialPort)
        target_link_libraries(QGC PUBLIC Qt6::SerialPort)
    endif()
endif()

#===========================================================================#

if(QGC_ENABLE_BLUETOOTH)
    find_package(Qt6 REQUIRED COMPONENTS Bluetooth)
    target_link_libraries(QGC PUBLIC Qt6::Bluetooth)
    target_sources(QGC
        PRIVATE
            BluetoothLink.cc
            BluetoothLink.h
    )
    target_compile_definitions(QGC PUBLIC QGC_ENABLE_BLUETOOTH)
endif()

#===========================================================================#

if(QGC_ZEROCONF_ENABLED)
    message(STATUS "Building QMDNSEngine")

    include(CPM)
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
        target_link_libraries(QGC PUBLIC qmdnsengine)
        target_compile_definitions(QGC PUBLIC QGC_ZEROCONF_ENABLED)
    endif()
endif()

# target_compile_definitions(QGC PUBLIC QABSTRACTSOCKET_DEBUG)
