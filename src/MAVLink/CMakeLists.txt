find_package(Qt6 REQUIRED COMPONENTS Core Gui QmlIntegration)

target_sources(QGC
    PRIVATE
        ImageProtocolManager.cc
        ImageProtocolManager.h
        MAVLinkFTP.cc
        MAVLinkFTP.h
        MAVLinkLib.h
        MAVLinkSigning.cc
        MAVLinkSigning.h
        MAVLinkStreamConfig.cc
        MAVLinkStreamConfig.h
        QGCMAVLink.cc
        QGCMAVLink.h
        StatusTextHandler.cc
        StatusTextHandler.h
        SysStatusSensorInfo.cc
        SysStatusSensorInfo.h
)

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::QmlIntegration
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

message(STATUS "Building MAVLink")

# TODO:
# include(CPM)
# CPMAddPackage(
#     NAME mavlink
#     GITHUB_REPOSITORY mavlink/mavlink
#     GIT_TAG master
#     OPTIONS
#         "MAVLINK_DIALECT all"
#         "MAVLINK_VERSION 2.0"
# )

include(CPM)
CPMAddPackage(
    NAME mavlink
    GIT_REPOSITORY ${QGC_MAVLINK_GIT_REPO}
    GIT_TAG ${QGC_MAVLINK_GIT_TAG}
)

# For QGC all dialects means common and development. Though use of development mavlink code should be restricted to debug builds only.
target_include_directories(QGC
    PUBLIC
        ${mavlink_SOURCE_DIR}
        ${mavlink_SOURCE_DIR}/all
        ${mavlink_SOURCE_DIR}/common
        ${mavlink_SOURCE_DIR}/development
)

# if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#   target_compile_options(comm PRIVATE -Wno-address-of-packed-member)
# endif()

add_subdirectory(LibEvents)
