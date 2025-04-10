set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageProtocolManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageProtocolManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkFTP.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkFTP.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkLib.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkSigning.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkSigning.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkStreamConfig.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkStreamConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMAVLink.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMAVLink.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StatusTextHandler.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/StatusTextHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SysStatusSensorInfo.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SysStatusSensorInfo.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

#===========================================================================#

# TODO:
# CPMAddPackage(
#     NAME mavlink
#     GITHUB_REPOSITORY ${QGC_MAVLINK_GIT_REPO}
#     GIT_TAG ${QGC_MAVLINK_GIT_TAG}
#     OPTIONS
#         "MAVLINK_DIALECT all"
#         "MAVLINK_VERSION 2.0"
# )

CPMAddPackage(
    NAME mavlink
    GIT_REPOSITORY ${QGC_MAVLINK_GIT_REPO}
    GIT_TAG ${QGC_MAVLINK_GIT_TAG}
)

# For QGC all dialects means common and development. Though use of development mavlink code should be restricted to debug builds only.
set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${mavlink_SOURCE_DIR}
    ${mavlink_SOURCE_DIR}/all
    ${mavlink_SOURCE_DIR}/common
    ${mavlink_SOURCE_DIR}/development
    CACHE INTERNAL "" FORCE
)

# if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#   target_compile_options(comm PRIVATE -Wno-address-of-packed-member)
# endif()

add_subdirectory(LibEvents)
