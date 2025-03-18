find_package(Qt6 REQUIRED COMPONENTS Core)

target_sources(QGC
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

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
