# Local GPS receiver: sessions, connection policy, and the receiver's status Facts.
target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSReceiver.cc
            GPSReceiver.h
            GPSReceiverConnectionPolicy.cc
            GPSReceiverConnectionPolicy.h
            GPSReceiverConnector.h
            GPSReceiverFactGroup.cc
            GPSReceiverFactGroup.h
            GPSReceiverMessages.cc
            GPSReceiverMessages.h
            GPSReceiverSession.cc
            GPSReceiverSession.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps
    PREFIX "/json/GPS"
    FILES GPSReceiverFact.json
)
