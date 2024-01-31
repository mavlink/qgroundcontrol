find_package(Threads REQUIRED)

if(CONFIG_UTM_ADAPTER)
    message(STATUS "UTMSP is Initialized")
    add_library(UTMSP
            UTMSPRestInterface.cpp
            UTMSPBlenderRestInterface.cpp
            UTMSPAuthorization.cpp
            UTMSPNetworkRemoteIDManager.cpp
            UTMSPAircraft.cpp
            UTMSPOperator.cpp
            UTMSPFlightPlanManager.cpp
            UTMSPServiceController.cpp
            UTMSPVehicle.cpp
            UTMSPManager.cpp
            utmsp.qrc
            )

    add_custom_target(UTMSPQml
        SOURCES
        UTMSPAdapterEditor.qml
        UTMSPMapVisuals.qml
        UTMSPActivationStatusBar.qml
        )

else()
    # If CONFIG_UTM_ADAPTER is not set, use utmsp_dummy.qrc
    message("Dummy is Initialized")

    add_library(UTMSP
            dummy/utmsp_dummy.qrc
    )

    target_include_directories(UTMSP PUBLIC dummy)

endif()

target_include_directories(UTMSP PUBLIC services )

target_link_libraries(UTMSP

PRIVATE
        libevents_parser
PUBLIC
        Qt6::Core
        Qt6::Location
        Qt6::Widgets
        Threads::Threads
        qgc
)

target_include_directories(UTMSP INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
