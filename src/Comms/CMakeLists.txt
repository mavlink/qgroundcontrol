set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/LinkConfiguration.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LinkConfiguration.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LinkInterface.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LinkInterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LinkManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LinkManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogReplayLink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LogReplayLink.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LogReplayLinkController.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/LogReplayLinkController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkProtocol.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkProtocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TCPLink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TCPLink.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UDPLink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/UDPLink.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QABSTRACTSOCKET_DEBUG)

#===========================================================================#

if(NOT QGC_NO_SERIAL_LINK)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/QGCSerialPortInfo.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/QGCSerialPortInfo.h
        ${CMAKE_CURRENT_SOURCE_DIR}/SerialLink.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/SerialLink.h
        ${CMAKE_CURRENT_SOURCE_DIR}/UdpIODevice.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/UdpIODevice.h
        CACHE INTERNAL "" FORCE
    )

    if(NOT ANDROID AND NOT IOS)
        set(QGC_LIBRARIES
            ${QGC_LIBRARIES}
            Qt6::SerialPort
            CACHE INTERNAL "" FORCE
        )
    endif()
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_NO_SERIAL_LINK)
endif()

#===========================================================================#

if(QGC_ENABLE_BLUETOOTH)
    set(QGC_SOURCES
        ${QGC_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/BluetoothLink.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/BluetoothLink.h
        CACHE INTERNAL "" FORCE
    )
    set(QGC_LIBRARIES
        ${QGC_LIBRARIES}
        Qt6::Bluetooth
        CACHE INTERNAL "" FORCE
    )
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_ENABLE_BLUETOOTH)
endif()

#===========================================================================#

if(QGC_ZEROCONF_ENABLED)
    CPMAddPackage(
        NAME qmdnsengine
        GITHUB_REPOSITORY nitroshare/qmdnsengine
        GIT_TAG master
        OPTIONS
            "BUILD_DOC OFF"
            "BUILD_EXAMPLES OFF"
            "BUILD_TESTS OFF"
    )

    set(QGC_LIBRARIES
        ${QGC_LIBRARIES}
        qmdnsengine
        CACHE INTERNAL "" FORCE
    )
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_ZEROCONF_ENABLED)
endif()

#===========================================================================#

add_subdirectory(AirLink)
add_subdirectory(MockLink)
