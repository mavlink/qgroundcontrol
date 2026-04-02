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

CPMAddPackage(
    NAME mavlink
    GIT_REPOSITORY ${QGC_MAVLINK_GIT_REPO}
    GIT_TAG ${QGC_MAVLINK_GIT_TAG}
    OPTIONS
        "MAVLINK_DIALECT ${QGC_MAVLINK_DIALECT}"
        "MAVLINK_VERSION ${QGC_MAVLINK_VERSION}"
)

set(_qgc_mavlink_requirements_file "${mavlink_SOURCE_DIR}/pymavlink/requirements.txt")
if(EXISTS "${_qgc_mavlink_requirements_file}")
    file(READ "${_qgc_mavlink_requirements_file}" _qgc_mavlink_requirements_content)
    string(REPLACE "\n# dev dependencies:\npytest<=7.4.4\nsyrupy; python_version>='3.6'\nwsproto\n" "\n" _qgc_mavlink_requirements_content "${_qgc_mavlink_requirements_content}")
    file(WRITE "${_qgc_mavlink_requirements_file}" "${_qgc_mavlink_requirements_content}")
    message(STATUS "QGC: Patched pymavlink requirements for offline-compatible mavgen build")
endif()

if(TARGET mavlink)
    add_dependencies(${CMAKE_PROJECT_NAME} mavlink)
    target_include_directories(${CMAKE_PROJECT_NAME}
        PRIVATE
            ${mavlink_BINARY_DIR}/include/mavlink
            ${mavlink_BINARY_DIR}/include/mavlink/${QGC_MAVLINK_DIALECT}
    )

    if(TARGET USVModule)
        add_dependencies(USVModule mavlink)
        target_include_directories(USVModule
            PRIVATE
                ${mavlink_BINARY_DIR}/include/mavlink
                ${mavlink_BINARY_DIR}/include/mavlink/${QGC_MAVLINK_DIALECT}
        )
    endif()
endif()



# ----------------------------------------------------------------------------
# LibEvents - MAVLink event handling
# ----------------------------------------------------------------------------
add_subdirectory(LibEvents)
