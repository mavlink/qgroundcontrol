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
        MAVLinkMessageType.h
        MAVLinkSigning.cc
        MAVLinkSigning.h
        MAVLinkSigningKeys.cc
        MAVLinkSigningKeys.h
        MAVLinkStreamConfig.cc
        MAVLinkStreamConfig.h
        QGCMAVLink.cc
        QGCMAVLink.h
        QGCMAVLinkTypes.h
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

if(mavlink_ADDED)
    add_dependencies(${CMAKE_PROJECT_NAME} mavlink)
    target_include_directories(${CMAKE_PROJECT_NAME}
        PRIVATE
            ${mavlink_BINARY_DIR}/include/mavlink
            ${mavlink_BINARY_DIR}/include/mavlink/${QGC_MAVLINK_DIALECT}
    )

    # Generate lightweight MAVLinkEnums.h (enum typedefs only, no message pack/unpack code)
    # plus MAVLinkEnumsQml.{h,cc} exposing every enum via Q_NAMESPACE/Q_ENUM_NS for QML.
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(MAVLINK_ENUMS_H      "${CMAKE_BINARY_DIR}/MAVLinkEnums.h")
    set(MAVLINK_ENUMS_QML_H  "${CMAKE_BINARY_DIR}/MAVLinkEnumsQml.h")
    set(MAVLINK_ENUMS_QML_CC "${CMAKE_BINARY_DIR}/MAVLinkEnumsQml.cc")
    set(MAVLINK_ENUMS_GENERATOR "${CMAKE_SOURCE_DIR}/tools/generators/mavlink_enums.py")
    add_custom_command(
        OUTPUT "${MAVLINK_ENUMS_H}" "${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}"
        COMMAND ${Python3_EXECUTABLE}
            "${MAVLINK_ENUMS_GENERATOR}"
            "${mavlink_BINARY_DIR}/include/mavlink"
            "${MAVLINK_ENUMS_H}"
            "${MAVLINK_ENUMS_QML_H}"
            "${MAVLINK_ENUMS_QML_CC}"
        DEPENDS mavlink "${MAVLINK_ENUMS_GENERATOR}"
        COMMENT "Generating MAVLinkEnums.h + MAVLinkEnumsQml.{h,cc}"
        VERBATIM
    )
    add_custom_target(mavlink_enums DEPENDS "${MAVLINK_ENUMS_H}" "${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}")
    add_dependencies(${CMAKE_PROJECT_NAME} mavlink_enums)
    # Ensure AUTOMOC runs after the generator
    set_property(TARGET ${CMAKE_PROJECT_NAME} APPEND PROPERTY AUTOGEN_TARGET_DEPENDS mavlink_enums)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}")

    # Register the QML anchor files so AUTOMOC picks up Q_NAMESPACE + Q_ENUM_NS
    set_source_files_properties(
        "${MAVLINK_ENUMS_QML_H}"
        "${MAVLINK_ENUMS_QML_CC}"
        PROPERTIES GENERATED TRUE
    )
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        "${MAVLINK_ENUMS_QML_H}"
        "${MAVLINK_ENUMS_QML_CC}"
    )
endif()

# ----------------------------------------------------------------------------
# LibEvents - MAVLink event handling
# ----------------------------------------------------------------------------
add_subdirectory(LibEvents)
