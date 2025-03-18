find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

target_sources(QGC
    PRIVATE
        FirmwarePlugin.cc
        FirmwarePlugin.h
        FirmwarePluginFactory.cc
        FirmwarePluginFactory.h
        FirmwarePluginManager.cc
        FirmwarePluginManager.h
)

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
        Qt6::Positioning
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# if(QGC_DISABLE_APM_MAVLINK)
#     target_compile_definitions(QGC PUBLIC QGC_NO_ARDUPILOT_DIALECT)
# endif()

add_subdirectory(APM)
add_subdirectory(PX4)
