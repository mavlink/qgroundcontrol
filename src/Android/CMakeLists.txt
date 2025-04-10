if(NOT ANDROID)
    return()
endif()

set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/AndroidInit.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AndroidInterface.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AndroidInterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AndroidSerial.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/AndroidSerial.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

add_subdirectory(qtandroidserialport)
