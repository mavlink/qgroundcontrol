# ============================================================================
# MAVLink Module
# MAVLink protocol implementation and communication handling
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ImageProtocolManager.cc
        ImageProtocolManager.h
        MAVLinkFTP.cc
        MAVLinkFTP.h
        MAVLinkLib.h
        MAVLinkSigning.cc
        MAVLinkSigning.h
        MAVLinkSigningKeys.cc
        MAVLinkSigningKeys.h
        MAVLinkStreamConfig.cc
        MAVLinkStreamConfig.h
        QGCMAVLink.cc
        QGCMAVLink.h
        StatusTextHandler.cc
        StatusTextHandler.h
        SysStatusSensorInfo.cc
        SysStatusSensorInfo.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# MAVLink Protocol Library Integration
# ============================================================================

message(STATUS "QGC: Building MAVLink")

if(NOT USE_SYSTEM_MAVLINK)
CPMAddPackage(
    NAME mavlink
    GIT_REPOSITORY ${QGC_MAVLINK_GIT_REPO}
    GIT_TAG ${QGC_MAVLINK_GIT_TAG}
    OPTIONS
        "MAVLINK_DIALECT ${QGC_MAVLINK_DIALECT}"
        "MAVLINK_VERSION ${QGC_MAVLINK_VERSION}"
)

else()
set(mavlink_SOURCE_DIR /usr/include/c_library_v2)
file(GLOB MAVLINK_SOURCES1 ${mavlink_SOURCE_DIR}/*)
file(GLOB MAVLINK_SOURCES2 ${mavlink_SOURCE_DIR}/all/*)
file(GLOB MAVLINK_SOURCES3 ${mavlink_SOURCE_DIR}/common/*)
file(GLOB MAVLINK_SOURCES4 ${mavlink_SOURCE_DIR}/development/*)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${MAVLINK_SOURCES1}
        ${MAVLINK_SOURCES2}
        ${MAVLINK_SOURCES3}
        ${MAVLINK_SOURCES4}
)
endif()

if(mavlink_ADDED)
    add_dependencies(${CMAKE_PROJECT_NAME} mavlink)
    target_include_directories(${CMAKE_PROJECT_NAME}
        PRIVATE
            ${mavlink_BINARY_DIR}/include/mavlink
            ${mavlink_BINARY_DIR}/include/mavlink/${QGC_MAVLINK_DIALECT}
    )
endif()

# ----------------------------------------------------------------------------
# LibEvents - MAVLink event handling
# ----------------------------------------------------------------------------
add_subdirectory(LibEvents)
