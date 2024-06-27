add_subdirectory(LibEvents)

find_package(Qt6 REQUIRED COMPONENTS Core Gui QmlIntegration)

qt_add_library(MAVLink STATIC
    ImageProtocolManager.cc
    ImageProtocolManager.h
    MAVLinkFTP.cc
    MAVLinkFTP.h
    MAVLinkLib.h
    MAVLinkStreamConfig.cc
    MAVLinkStreamConfig.h
    QGCMAVLink.cc
    QGCMAVLink.h
    StatusTextHandler.cc
    StatusTextHandler.h
    SysStatusSensorInfo.cc
    SysStatusSensorInfo.h
)

target_link_libraries(MAVLink
    PRIVATE
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::QmlIntegration
)

target_include_directories(MAVLink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

############# MAVLink

include(FetchContent)
FetchContent_Declare(mavlink
        GIT_REPOSITORY https://github.com/mavlink/c_library_v2.git
        GIT_TAG 878a0660666bbb7b982bad4f0d10fad88b1b82d1
)
FetchContent_MakeAvailable(mavlink)

target_include_directories(MAVLink
    PUBLIC
        ${mavlink_SOURCE_DIR}
        ${mavlink_SOURCE_DIR}/all
        ${mavlink_SOURCE_DIR}/common
)

# if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#   target_compile_options(comm PRIVATE -Wno-address-of-packed-member)
# endif()
