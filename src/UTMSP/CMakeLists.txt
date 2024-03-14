find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Threads REQUIRED)

qt_add_library(UTMSP STATIC)

option(CONFIG_UTM_ADAPTER "Enable UTM Adapter" OFF) # TODO: Make this QGC_CONFIG_UTM_ADAPTER
if(CONFIG_UTM_ADAPTER)
    message(STATUS "UTMSP is Initialized")
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

    target_sources(UTMSP
        PRIVATE
            ${UTMSP_RESOURCES}
    )
endif()

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
