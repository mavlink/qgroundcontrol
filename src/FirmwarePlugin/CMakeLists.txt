set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/FirmwarePlugin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FirmwarePlugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FirmwarePluginFactory.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FirmwarePluginFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FirmwarePluginManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FirmwarePluginManager.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

add_subdirectory(APM)
add_subdirectory(PX4)
