if(NOT QGC_UTM_ADAPTER)
    # qt_add_resources(UTMSP_RESOURCES dummy/utmsp_dummy.qrc)
    # target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${UTMSP_RESOURCES})

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

set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPAircraft.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPAircraft.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPAuthorization.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPAuthorization.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPBlenderRestInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPBlenderRestInterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPFlightDetails.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPFlightDetails.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPFlightPlanManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPFlightPlanManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPLogger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPNetworkRemoteIDManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPNetworkRemoteIDManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPOperator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPOperator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPRestInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPRestInterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPServiceController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPServiceController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPVehicle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UTMSPVehicle.h
    CACHE INTERNAL "" FORCE
)

# CPMAddPackage(
#     NAME nlohmann_json
#     VERSION 3.11.3
#     GITHUB_REPOSITORY nlohmann/json
# )

# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE nlohmann_json)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/services
    CACHE INTERNAL "" FORCE
)

set(QGC_DEFINES
    ${QGC_DEFINES}
    QGC_UTM_ADAPTER
    CACHE INTERNAL "" FORCE
)

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
