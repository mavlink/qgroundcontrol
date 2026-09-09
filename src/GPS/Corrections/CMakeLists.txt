qgc_add_gps_component(QGCGPSCorrections COMPONENT Corrections LOGGING_TARGET QGCLogging)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSCorrectionFrame.h RTCMMavlink.cc RTCMMavlink.h RTCMUdpInput.cc
                                             RTCMUdpInput.h
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core QGCGPSCorrections)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSCorrectionManager.cc GPSCorrectionManager.h)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSMavlinkOutput.cc GPSMavlinkOutput.h)
