find_package(Qt6 REQUIRED COMPONENTS Core)

set(UTMSP_RESOURCES)
qt_add_library(UTMSP STATIC)

option(QGC_UTM_ADAPTER "Enable UTM Adapter" OFF)
if(QGC_UTM_ADAPTER)
    message(STATUS "UTMSP is Initialized")

    if(NOT nlohmann_json_FOUND)
        message(STATUS "UTMSP: Fetching nlohmann/json")
        include(FetchContent)
        FetchContent_Declare(json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG v3.11.3
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(json)
    endif()

    find_package(Qt6 REQUIRED COMPONENTS Network Positioning Qml)
    find_package(Threads REQUIRED)

    qt_add_resources(UTMSP_RESOURCES utmsp.qrc)
    target_sources(UTMSP
        PRIVATE
            UTMSPAircraft.cpp
            UTMSPAircraft.h
            UTMSPAuthorization.cpp
            UTMSPAuthorization.h
            UTMSPBlenderRestInterface.cpp
            UTMSPBlenderRestInterface.h
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
            # ${UTMSP_RESOURCES}
    )

    target_link_libraries(UTMSP
        PRIVATE
            boost_system
            nlohmann_json
            Threads::Threads
            Qt6::Network
            Comms
        PUBLIC
            Qt6::Core
            Qt6::Positioning
            QGC
    )

    target_include_directories(UTMSP
        PRIVATE
            services
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_compile_definitions(UTMSP PUBLIC CONFIG_UTM_ADAPTER)

    # qt_add_qml_module(UTMSP
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

else()
    message(STATUS "UTMSP: Dummy is Initialized")

    qt_add_resources(UTMSP_RESOURCES dummy/utmsp_dummy.qrc)
    target_sources(UTMSP PRIVATE ${UTMSP_RESOURCES})

    # qt_add_qml_module(UTMSP
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

endif()
