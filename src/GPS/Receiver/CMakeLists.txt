# Local GPS receiver: sessions, connection policy, serial devices, the receiver's status Facts and what its views
# offer for each receiver family.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSReceiver.cc
        GPSReceiver.h
        GPSReceiverConnectionPolicy.cc
        GPSReceiverConnectionPolicy.h
        GPSReceiverConnector.h
        GPSReceiverFactGroup.cc
        GPSReceiverFactGroup.h
        GPSReceiverMessages.cc
        GPSReceiverMessages.h
        GPSReceiverPresentation.cc
        GPSReceiverPresentation.h
        GPSReceiverSerialPorts.cc
        GPSReceiverSerialPorts.h
        GPSReceiverSession.cc
        GPSReceiverSession.h
        GPSSatelliteModel.cc
        GPSSatelliteModel.h
)
# The serial devices behind GPSReceiverSerialPorts, which is the whole serial side of a build without serial links.
if(NOT QGC_NO_SERIAL_LINK)
    target_sources(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
            GPSReceiverSystemSerialPorts.cc
            GPSReceiverSystemSerialPorts.h
    )
endif()
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps
    PREFIX "/json/GPS"
    FILES
        GPSReceiverFact.json
)
