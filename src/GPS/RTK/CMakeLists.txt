# ============================================================================
# RTK Module
# RTK base station, RTCM correction forwarding, and survey-in
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSPositionSource.cc
        GPSPositionSource.h
        GPSRtk.cc
        GPSRtk.h
        GPSSatelliteInfoSource.cc
        GPSSatelliteInfoSource.h
        GPSRTKFactGroup.cc
        GPSRTKFactGroup.h
        RTCMMavlink.cc
        RTCMMavlink.h
        GPSDeviceFlags.cc
        GPSDeviceFlags.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# RTK QML Module
# ----------------------------------------------------------------------------
qt_add_library(GPSRTKModule STATIC)

set_source_files_properties(GPSSNRThresholds.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(GPSRTKModule
    URI QGroundControl.GPS.RTK
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        FixStatusDot.qml
        GPSIndicator.qml
        GPSIndicatorPage.qml
        GPSSNRThresholds.qml
        GPSStatusBlock.qml
        RTKBaseStationMapItem.qml
        RTKBaseStationStatus.qml
        RTKCorrectionsStatusBlock.qml
        RTKGPSIndicator.qml
        SignalIntegrityBlock.qml
        SignalStrengthBar.qml
        VehicleGPSIndicator.qml
    RESOURCES
        Images/Gps.svg
        Images/GpsAuthentication.svg
        Images/GpsInterference.svg
        Images/RTK.svg
    NO_PLUGIN
)

# ----------------------------------------------------------------------------
# RTK Fact Metadata
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} json_gps_rtk
    PREFIX "/json/Vehicle"
    FILES GPSRTKFact.json
)
