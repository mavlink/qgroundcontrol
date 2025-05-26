set(UTMSP_RESOURCES)

if(NOT QGC_UTM_ADAPTER)
    # qt_add_resources(UTMSP_RESOURCES dummy/utmsp_dummy.qrc)

    # qt_add_qml_module(QGC
    #     URI QGroundControl.UTMSP
    #     VERSION 1.0
    #     QML_FILES
    #         dummy/UTMSPActivationStatusBar.qml
    #         dummy/UTMSPAdapterEditor.qml
    #         dummy/UTMSPMapPolygonVisuals.qml
    #         dummy/UTMSPMapVisuals.qml
    #         dummy/UTMSPStateStorage.qml
    #     OUTPUT_TARGETS UTMSP_targets
    #     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
    #     IMPORTS
    #         QtQuick
    #         QGroundControl.UTMSP
    # )
    return()
endif()

# qt_add_resources(UTMSP_RESOURCES utmsp.qrc)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
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

# qt_add_qml_module(QGC
#     URI QGroundControl.UTMSP
#     VERSION 1.0
#     QML_FILES
#         UTMSPActivationStatusBar.qml
#         UTMSPAdapterEditor.qml
#         UTMSPMapPolygonVisuals.qml
#         UTMSPMapVisuals.qml
#         UTMSPStateStorage.qml
#     OUTPUT_TARGETS UTMSP_targets
#     RESOURCES
#         images/date.svg
#         images/green.png
#         images/load.gif
#         images/red.png
#         images/time.svg
#         images/timer.gif
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Layouts
#         QtQuick.Dialogs
#         QtQuick.Controls
#         QtPositioning
#         QtLocation

#         QGroundControl
#         QGroundControl.FlightMap
#         QGroundControl.ScreenTools
#         QGroundControl.Controls
#         QGroundControl.FactSystem
#         QGroundControl.FactControls
#         QGroundControl.Palette
#         QGroundControl.Controllers
#         QGroundControl.ShapeFileHelper
#         QGroundControl.UTMSP
#         QGroundControl.FlightDisplay
#         QGroundControl.MultiVehicleManager
# )
