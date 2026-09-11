target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSManager.cc
            GPSManager.h
            GPSSourceBindings.cc
            GPSSourceBindings.h
            GPSConnectionSettingsController.cc
            GPSConnectionSettingsController.h
            GPSSettings.cc
            GPSSettings.h
            GPSMavlinkOutput.cc
            GPSMavlinkOutput.h
            RTCMMavlink.cc
            RTCMMavlink.h
            RTCMUdpInput.cc
            RTCMUdpInput.h
            GPSCorrectionManager.cc
            GPSCorrectionManager.h
            NTRIPManager.cc
            NTRIPManager.h
            NTRIPConnectionSettings.h
            NTRIPConnectionSettings.cc
            PositionManager.cpp
            PositionManager.h
            SimulatedPosition.cc
            SimulatedPosition.h
            GPSBaseReferenceSave.cc
            GPSBaseReferenceSave.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    PRIVATE QGCGPSDriver
            QGCGPSReceiver
            QGCGPSConnections
            QGCGPSModels
            QGCGPSPositioning
            QGCGPSNTRIPNetwork
            QGCGPSRecordingController
)
if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSSerialPortRegistry.cc GPSSerialPortRegistry.h)
endif()
