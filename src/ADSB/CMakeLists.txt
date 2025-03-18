find_package(Qt6 REQUIRED COMPONENTS Core Network Positioning QmlIntegration)

target_sources(QGC
    PRIVATE
        ADSBTCPLink.cc
        ADSBTCPLink.h
        ADSBVehicle.cc
        ADSBVehicle.h
        ADSBVehicleManager.cc
        ADSBVehicleManager.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::Network
    PUBLIC
        Qt6::Core
        Qt6::Positioning
        Qt6::QmlIntegration
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
