target_sources(${CMAKE_PROJECT_NAME}
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

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
