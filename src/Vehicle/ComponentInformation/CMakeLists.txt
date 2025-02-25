find_package(Qt6 REQUIRED COMPONENTS Core)

target_sources(Vehicle
    PRIVATE
        CompInfo.cc
        CompInfo.h
        CompInfoActuators.cc
        CompInfoActuators.h
        CompInfoEvents.cc
        CompInfoEvents.h
        CompInfoGeneral.cc
        CompInfoGeneral.h
        CompInfoParam.cc
        CompInfoParam.h
        ComponentInformationCache.cc
        ComponentInformationCache.h
        ComponentInformationManager.cc
        ComponentInformationManager.h
        ComponentInformationTranslation.cc
        ComponentInformationTranslation.h
)

target_link_libraries(Vehicle
    PRIVATE
        FirmwarePlugin
        QGC
    PUBLIC
        Qt6::Core
        FactSystem
        MAVLink
        Utilities
)

target_include_directories(Vehicle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
