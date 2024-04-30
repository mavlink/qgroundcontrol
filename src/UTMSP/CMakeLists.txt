find_package(Qt6 REQUIRED COMPONENTS Core)

option(QGC_UTM_ADAPTER "Enable UTM Adapter" OFF)
if(QGC_UTM_ADAPTER)
    message(STATUS "UTMSP is Initialized")

    find_package(Qt6 REQUIRED COMPONENTS Qml Location Widgets)
    find_package(Threads REQUIRED)

    set(UTMSP_RESOURCES)
    qt_add_resources(UTMSP_RESOURCES utmsp.qrc)

    qt_add_library(UTMSP STATIC
        PRIVATE
            UTMSPAircraft.cpp
            UTMSPAircraft.h
            UTMSPAuthorization.cpp
            UTMSPAuthorization.h
            UTMSPBlenderRestInterface.cpp
            UTMSPBlenderRestInterface.h
            UTMSPFlightPlanManager.cpp
            UTMSPFlightPlanManager.h
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
            ${UTMSP_RESOURCES}
    )

    target_link_libraries(UTMSP
        PRIVATE
            nlohmann_json
        PUBLIC
            Qt6::Core
            Qt6::Location
            Qt6::Widgets
            Threads::Threads
            qgc
    )

    target_include_directories(UTMSP
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            services
    )

    target_compile_definitions(UTMSP PUBLIC CONFIG_UTM_ADAPTER)

    add_custom_target(UTMSPQml
        SOURCES
            UTMSPActivationStatusBar.qml
            UTMSPAdapterEditor.qml
            UTMSPMapVisuals.qml
    )

else()
    message(STATUS "UTMSP: Dummy is Initialized")

    set(UTMSP_RESOURCES)
    qt_add_resources(UTMSP_RESOURCES dummy/utmsp_dummy.qrc)

    qt_add_library(UTMSP STATIC ${UTMSP_RESOURCES})
endif()
