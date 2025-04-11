set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfo.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfoActuators.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfoActuators.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfoEvents.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfoEvents.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfoGeneral.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfoGeneral.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfoParam.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CompInfoParam.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationCache.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationCache.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationTranslation.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ComponentInformationTranslation.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)
