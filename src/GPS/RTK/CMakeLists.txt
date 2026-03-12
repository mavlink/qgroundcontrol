# ============================================================================
# RTK Module
# RTK base station, RTCM correction forwarding, and survey-in
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSRtk.cc
        GPSRtk.h
        GPSRTKFactGroup.cc
        GPSRTKFactGroup.h
        RTCMMavlink.cc
        RTCMMavlink.h
        RTCMRouter.cc
        RTCMRouter.h
        RTKSatelliteModel.cc
        RTKSatelliteModel.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# RTK QML Module
# ----------------------------------------------------------------------------
qt_add_library(GPSRTKModule STATIC)

qt_add_qml_module(GPSRTKModule
    URI QGroundControl.GPS.RTK
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        RTKBaseStationMapItem.qml
        RTKBaseStationStatus.qml
        RTKSettings.qml
    NO_PLUGIN
)

# ----------------------------------------------------------------------------
# RTK Fact Metadata
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} json_gps_rtk
    PREFIX "/json/Vehicle"
    FILES GPSRTKFact.json
)
