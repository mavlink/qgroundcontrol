if(NOT ANDROID)
    return()
endif()

set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/qserialport.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qserialport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/qserialport_android.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qserialport_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/qserialportglobal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/qserialportinfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qserialportinfo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/qserialportinfo_p.h
    ${CMAKE_CURRENT_SOURCE_DIR}/qserialportinfo_android.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/qtserialportexports.h
    ${CMAKE_CURRENT_SOURCE_DIR}/qtserialportversion.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QIODEVICE_DEBUG)
