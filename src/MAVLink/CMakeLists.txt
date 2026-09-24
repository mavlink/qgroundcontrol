# ============================================================================
# MAVLink Module
# MAVLink protocol implementation and communication handling
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE ImageProtocolManager.cc
            ImageProtocolManager.h
            MAVLinkFTP.cc
            MAVLinkFTP.h
            MAVLinkLib.h
            MAVLinkMessageType.h
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

if(QGC_USE_SYSTEM_LIBS OR QGC_SYSTEM_LIBS_ONLY)
    # System mavlink: generated headers come from c_library_v2, the source
    # files (message_definitions XML + component_metadata JSON) from mavlink-src.
    set(QGC_SYSTEM_MAVLINK_INCLUDE_DIR "/usr/include/c_library_v2" CACHE PATH
        "System mavlink generated headers (c_library_v2)")
    set(QGC_SYSTEM_MAVLINK_SOURCE_DIR "/usr/src/mavlink" CACHE PATH
        "System mavlink source tree (mavlink-src)")
    set(QGC_MAVLINK_INCLUDE_DIR "${QGC_SYSTEM_MAVLINK_INCLUDE_DIR}")
    set(QGC_MAVLINK_DIALECT_INCLUDE_DIR "${QGC_SYSTEM_MAVLINK_INCLUDE_DIR}/${QGC_MAVLINK_DIALECT}")
    set(QGC_MAVLINK_MESSAGE_DEFINITIONS_DIR "${QGC_SYSTEM_MAVLINK_SOURCE_DIR}/message_definitions/v1.0")
    set(QGC_MAVLINK_COMPONENT_METADATA_DIR "${QGC_SYSTEM_MAVLINK_SOURCE_DIR}/component_metadata")
    set(QGC_MAVLINK_CPM_TARGET "")
else()
    include(CPMPatchCache)

    set(_mavlink_patch "${CMAKE_CURRENT_LIST_DIR}/mavlink-deterministic-mavgen.patch")
    qgc_cpm_patch_cache_key(
        _mavlink_cache_key
        GIT_REPOSITORY
        "${QGC_MAVLINK_GIT_REPO}"
        GIT_TAG
        "${QGC_MAVLINK_GIT_TAG}"
        PATCHES
        "${_mavlink_patch}"
    )

    CPMAddPackage(
        NAME mavlink
        GIT_REPOSITORY ${QGC_MAVLINK_GIT_REPO}
        GIT_TAG ${QGC_MAVLINK_GIT_TAG}
        # CPM's default source-cache key does not hash PATCHES file contents.
        CUSTOM_CACHE_KEY "${_mavlink_cache_key}"
        OPTIONS "MAVLINK_DIALECT ${QGC_MAVLINK_DIALECT}" "MAVLINK_VERSION ${QGC_MAVLINK_VERSION}"
        # Use the locked generator environment and make mavgen hashes deterministic.
        PATCHES "${_mavlink_patch}"
    )

    qgc_require_cpm_added(mavlink)

    add_dependencies(${CMAKE_PROJECT_NAME} mavlink)
    set(QGC_MAVLINK_INCLUDE_DIR "${mavlink_BINARY_DIR}/include/mavlink")
    set(QGC_MAVLINK_DIALECT_INCLUDE_DIR "${mavlink_BINARY_DIR}/include/mavlink/${QGC_MAVLINK_DIALECT}")
    set(QGC_MAVLINK_MESSAGE_DEFINITIONS_DIR "${mavlink_SOURCE_DIR}/message_definitions/v1.0")
    set(QGC_MAVLINK_COMPONENT_METADATA_DIR "${mavlink_SOURCE_DIR}/component_metadata")
    set(QGC_MAVLINK_CPM_TARGET mavlink)
endif()
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${QGC_MAVLINK_INCLUDE_DIR}
                                                         ${QGC_MAVLINK_DIALECT_INCLUDE_DIR}
)

# Generate lightweight MAVLinkEnums.h (enum typedefs only, no message pack/unpack code)
# plus MAVLinkEnumsQml.{h,cc} exposing every enum via Q_NAMESPACE/Q_ENUM_NS for QML.
set(MAVLINK_ENUMS_H "${CMAKE_BINARY_DIR}/MAVLinkEnums.h")
set(MAVLINK_ENUMS_QML_H "${CMAKE_BINARY_DIR}/MAVLinkEnumsQml.h")
set(MAVLINK_ENUMS_QML_CC "${CMAKE_BINARY_DIR}/MAVLinkEnumsQml.cc")
set(MAVLINK_ENUMS_GENERATOR "${CMAKE_SOURCE_DIR}/tools/generators/mavlink_enums.py")
add_custom_command(
    OUTPUT "${MAVLINK_ENUMS_H}" "${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}"
    COMMAND Python3::Interpreter "${MAVLINK_ENUMS_GENERATOR}" "${QGC_MAVLINK_INCLUDE_DIR}"
            "${MAVLINK_ENUMS_H}" "${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}"
    DEPENDS ${QGC_MAVLINK_CPM_TARGET} "${MAVLINK_ENUMS_GENERATOR}" "${CMAKE_SOURCE_DIR}/tools/common/io.py"
    COMMENT "Generating MAVLinkEnums.h + MAVLinkEnumsQml.{h,cc}"
    VERBATIM
)
add_custom_target(
    mavlink_enums
    DEPENDS "${MAVLINK_ENUMS_H}" "${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}"
    COMMENT "Checking generated MAVLink enum bindings"
)
add_dependencies(${CMAKE_PROJECT_NAME} mavlink_enums)
# Ensure AUTOMOC runs after the generator
set_property(
    TARGET ${CMAKE_PROJECT_NAME}
    APPEND
    PROPERTY AUTOGEN_TARGET_DEPENDS mavlink_enums
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}")

# Register the QML anchor files so AUTOMOC picks up Q_NAMESPACE + Q_ENUM_NS
set_source_files_properties("${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}" PROPERTIES GENERATED TRUE)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}")

# Generate MAVLinkInstanceFields.h (message ID → instance field name mapping)
set(MAVLINK_INSTANCE_FIELDS_H "${CMAKE_BINARY_DIR}/MAVLinkInstanceFields.h")
set(MAVLINK_INSTANCE_FIELDS_GENERATOR "${CMAKE_SOURCE_DIR}/tools/generators/mavlink_instance_fields.py")
set(MAVLINK_XML_DIR "${QGC_MAVLINK_MESSAGE_DEFINITIONS_DIR}")
add_custom_command(
    OUTPUT "${MAVLINK_INSTANCE_FIELDS_H}"
    COMMAND Python3::Interpreter "${MAVLINK_INSTANCE_FIELDS_GENERATOR}" "${MAVLINK_XML_DIR}" "${QGC_MAVLINK_DIALECT}"
            "${MAVLINK_INSTANCE_FIELDS_H}"
    DEPENDS ${QGC_MAVLINK_CPM_TARGET} "${MAVLINK_INSTANCE_FIELDS_GENERATOR}" "${CMAKE_SOURCE_DIR}/tools/common/io.py"
            "${CMAKE_SOURCE_DIR}/tools/common/xml.py"
    COMMENT "Generating MAVLinkInstanceFields.h"
    VERBATIM
)
add_custom_target(
    mavlink_instance_fields
    DEPENDS "${MAVLINK_INSTANCE_FIELDS_H}"
    COMMENT "Checking generated MAVLink instance fields"
)
add_dependencies(${CMAKE_PROJECT_NAME} mavlink_instance_fields)

# Bundle the upstream component_metadata JSON schemas (referenced from the CPM dir, never
# vendored) so ComponentInformation validates metadata against the pinned QGC_MAVLINK_GIT_TAG.
set(MAVLINK_COMPONENT_METADATA_DIR "${QGC_MAVLINK_COMPONENT_METADATA_DIR}")
set(MAVLINK_SCHEMA_FILES
    "${MAVLINK_COMPONENT_METADATA_DIR}/parameter.schema.json"
    "${MAVLINK_COMPONENT_METADATA_DIR}/general.schema.json"
    "${MAVLINK_COMPONENT_METADATA_DIR}/actuators.schema.json"
    "${MAVLINK_COMPONENT_METADATA_DIR}/peripherals.schema.json"
    "${MAVLINK_COMPONENT_METADATA_DIR}/translation.schema.json"
)
qt_add_resources(
    ${CMAKE_PROJECT_NAME} mavlink_component_metadata_schemas
    PREFIX "/json/component_metadata"
    BASE "${MAVLINK_COMPONENT_METADATA_DIR}"
    FILES ${MAVLINK_SCHEMA_FILES}
)

# ----------------------------------------------------------------------------
# LibEvents - MAVLink event handling
# ----------------------------------------------------------------------------
add_subdirectory(LibEvents)
add_subdirectory(Signing)
