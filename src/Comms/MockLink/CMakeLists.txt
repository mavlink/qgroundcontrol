if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MockConfiguration.cc
        MockConfiguration.h
        MockLink.cc
        MockLink.h
        MockLinkFTP.cc
        MockLinkFTP.h
        MockLinkWorker.cc
        MockLinkWorker.h
        MockLinkMissionItemHandler.cc
        MockLinkMissionItemHandler.h
)

# Add resource files
set_source_files_properties(MockLink.General.MetaData.json PROPERTIES QT_RESOURCE_ALIAS General.MetaData.json)
set_source_files_properties(MockLink.General.MetaData.json.xz PROPERTIES QT_RESOURCE_ALIAS General.MetaData.json.xz)
set_source_files_properties(MockLink.Parameter.MetaData.json.xz PROPERTIES QT_RESOURCE_ALIAS Parameter.MetaData.json.xz)
set_source_files_properties(MockLink.Parameter.MetaData.json PROPERTIES QT_RESOURCE_ALIAS Parameter.MetaData.json)
set_source_files_properties(Mocklink.Arduplane.params.ftp.bin PROPERTIES QT_RESOURCE_ALIAS Arduplane.params.ftp.bin)

qt_add_resources(${CMAKE_PROJECT_NAME} mocklink_resources
    PREFIX "/MockLink"
    FILES
        PX4MockLink.params
        MockLink.General.MetaData.json
        MockLink.General.MetaData.json.xz
        MockLink.Parameter.MetaData.json.xz
        MockLink.Parameter.MetaData.json
        Mocklink.Arduplane.params.ftp.bin
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
