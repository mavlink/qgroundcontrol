find_package(Qt6 REQUIRED COMPONENTS Core Network Positioning QmlIntegration)

qt_add_library(ADSB STATIC
    ADSBTCPLink.cc
    ADSBTCPLink.h
    ADSBVehicle.cc
    ADSBVehicle.h
    ADSBVehicleManager.cc
    ADSBVehicleManager.h
)

target_link_libraries(ADSB
    PRIVATE
        Qt6::Network
        QGC
        Settings
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Positioning
        Qt6::QmlIntegration
        QmlControls
)

target_include_directories(ADSB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
