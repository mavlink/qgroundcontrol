# ============================================================================
# UTMSP Module
# Unmanned Traffic Management Service Provider integration
# ============================================================================

qt_add_library(UTMSPModule STATIC)

# ----------------------------------------------------------------------------
# Dummy UTMSP Module (when adapter is disabled)
# ----------------------------------------------------------------------------
if(NOT QGC_UTM_ADAPTER)
    set_source_files_properties(dummy/UTMSPStateStorage.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

    qt_add_qml_module(UTMSPModule
        URI QGroundControl.UTMSP
        VERSION 1.0
        RESOURCE_PREFIX /qml
        QML_FILES
            dummy/UTMSPActivationStatusBar.qml
            dummy/UTMSPAdapterEditor.qml
            dummy/UTMSPFlightStatusIndicator.qml
            dummy/UTMSPMapPolygonVisuals.qml
            dummy/UTMSPMapVisuals.qml
            dummy/UTMSPNotificationSlider.qml
            dummy/UTMSPStateStorage.qml
        NO_PLUGIN
    )

    return()
endif()

# ----------------------------------------------------------------------------
# Full UTMSP Adapter Implementation
# ----------------------------------------------------------------------------

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        services/dispatcher.h
        UTMSPAircraft.cpp
        UTMSPAircraft.h
        UTMSPAuthorization.cpp
        UTMSPAuthorization.h
        UTMSPBlenderRestInterface.cpp
        UTMSPBlenderRestInterface.h
        UTMSPFlightDetails.cpp
        UTMSPFlightDetails.h
        UTMSPFlightPlanManager.cpp
        UTMSPFlightPlanManager.h
        UTMSPLogger.h
        UTMSPManager.cpp
        UTMSPManager.h
        UTMSPNetworkRemoteIDManager.cpp
        UTMSPNetworkRemoteIDManager.h
        UTMSPOperator.cpp
        UTMSPOperator.h
        UTMSPRestInterface.cpp
        UTMSPRestInterface.h
        UTMSPServiceController.cpp
        UTMSPServiceController.h
        UTMSPVehicle.cpp
        UTMSPVehicle.h
)

# ----------------------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------------------
# TODO: Enable nlohmann_json when needed
# CPMFindPackage(
#     NAME nlohmann_json
#     VERSION 3.11.3
#     GITHUB_REPOSITORY nlohmann/json
# )

find_package(Threads REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        # nlohmann_json
        Threads::Threads
)

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        services
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_UTM_ADAPTER)

# ----------------------------------------------------------------------------
# UTMSP QML Module
# ----------------------------------------------------------------------------
set_source_files_properties(UTMSPStateStorage.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(UTMSPModule
    URI QGroundControl.UTMSP
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        UTMSPActivationStatusBar.qml
        UTMSPAdapterEditor.qml
        UTMSPFlightStatusIndicator.qml
        UTMSPMapPolygonVisuals.qml
        UTMSPMapVisuals.qml
        UTMSPNotificationSlider.qml
        UTMSPStateStorage.qml
    RESOURCES
        images/date.svg
        images/green.png
        images/green_led.png
        images/load.gif
        images/orange_led.png
        images/pale_green.png
        images/parrot_green.png
        images/red.png
        images/red_led.png
        images/time.svg
        images/timer.gif
        images/yellow_led.png
    NO_PLUGIN
)
