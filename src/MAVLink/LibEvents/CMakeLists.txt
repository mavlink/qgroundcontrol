set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/EventHandler.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/EventHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/HealthAndArmingCheckReport.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/HealthAndArmingCheckReport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/logging.cpp
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

#===========================================================================#

CPMAddPackage(
    NAME libevents
    GITHUB_REPOSITORY mavlink/libevents
    GIT_TAG main
    SOURCE_SUBDIR libs/cpp
)

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    libevents
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${libevents_SOURCE_DIR}/libs/cpp
    CACHE INTERNAL "" FORCE
)
