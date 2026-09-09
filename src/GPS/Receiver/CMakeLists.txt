qgc_add_gps_component(QGCGPSReceiver COMPONENT Receiver LOGGING_TARGET QGCLogging)

# Discovery integrates the application serial-port registry; worker sessions remain in the standalone library.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverAutoConnect.cc GPSReceiverAutoConnect.h)
if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverAutoConnectSerial.cc)
endif()

# Saved settings and presentation use Facts; worker sessions remain independent of the application.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSConnectionSettings.cc GPSConnectionSettings.h
                                             GPSReceiverSettingsPresentation.cc GPSReceiverSettingsPresentation.h
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiver.cc GPSReceiver.h GPSReceiverFactGroup.cc
                                             GPSReceiverFactGroup.h
)
qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_receiver
    PREFIX "/json/Vehicle"
    FILES GPSReceiverFact.json
)
