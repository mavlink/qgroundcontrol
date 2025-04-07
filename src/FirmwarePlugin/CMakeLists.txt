target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        FirmwarePlugin.cc
        FirmwarePlugin.h
        FirmwarePluginFactory.cc
        FirmwarePluginFactory.h
        FirmwarePluginManager.cc
        FirmwarePluginManager.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# if(QGC_DISABLE_APM_MAVLINK)
#     target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_NO_ARDUPILOT_DIALECT)
# endif()

add_subdirectory(APM)
add_subdirectory(PX4)
