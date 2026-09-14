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

if(QGC_MAVLINK_PROVIDER)
    include("${QGC_MAVLINK_PROVIDER}")
else()
    include(CPMPatchCache)

    set(_mavlink_patch "${CMAKE_CURRENT_LIST_DIR}/mavlink-deterministic-mavgen.patch")
    qgc_cpm_patch_cache_key(
        _mavlink_cache_key
        GIT_REPOSITORY "${QGC_MAVLINK_GIT_REPO}"
        GIT_TAG "${QGC_MAVLINK_GIT_TAG}"
        PATCHES "${_mavlink_patch}"
    )

    CPMAddPackage(
        NAME mavlink
        GIT_REPOSITORY ${QGC_MAVLINK_GIT_REPO}
        GIT_TAG ${QGC_MAVLINK_GIT_TAG}
        # CPM's default source-cache key does not hash PATCHES file contents.
        CUSTOM_CACHE_KEY "${_mavlink_cache_key}"
        OPTIONS
            "MAVLINK_DIALECT ${QGC_MAVLINK_DIALECT}"
            "MAVLINK_VERSION ${QGC_MAVLINK_VERSION}"
        # Retry upstream's build-time pip install and make mavgen hashes deterministic.
        PATCHES "${_mavlink_patch}"
    )

    qgc_require_cpm_added(mavlink)

    set(QGC_MAVLINK_GENERATED_DIR "${mavlink_BINARY_DIR}/include/mavlink")
    set(QGC_MAVLINK_XML_DIR "${mavlink_SOURCE_DIR}/message_definitions/v1.0")
    set(QGC_MAVLINK_COMPONENT_METADATA_DIR "${mavlink_SOURCE_DIR}/component_metadata")
endif()

if(NOT TARGET mavlink
   OR NOT QGC_MAVLINK_GENERATED_DIR
   OR NOT QGC_MAVLINK_XML_DIR
   OR NOT QGC_MAVLINK_COMPONENT_METADATA_DIR
)
    message(FATAL_ERROR
        "QGC: MAVLink provider must define the mavlink target, generated header directory, "
        "XML directory, and component metadata directory"
    )
endif()

add_dependencies(${CMAKE_PROJECT_NAME} mavlink)
target_include_directories(${CMAKE_PROJECT_NAME}
                           PRIVATE ${QGC_MAVLINK_GENERATED_DIR}
                                   ${QGC_MAVLINK_GENERATED_DIR}/${QGC_MAVLINK_DIALECT}
)

# Generate lightweight MAVLinkEnums.h (enum typedefs only, no message pack/unpack code)
# plus MAVLinkEnumsQml.{h,cc} exposing every enum via Q_NAMESPACE/Q_ENUM_NS for QML.
set(MAVLINK_ENUMS_H "${CMAKE_BINARY_DIR}/MAVLinkEnums.h")
set(MAVLINK_ENUMS_QML_H "${CMAKE_BINARY_DIR}/MAVLinkEnumsQml.h")
set(MAVLINK_ENUMS_QML_CC "${CMAKE_BINARY_DIR}/MAVLinkEnumsQml.cc")
set(MAVLINK_ENUMS_GENERATOR "${CMAKE_SOURCE_DIR}/tools/generators/mavlink_enums.py")
add_custom_command(
    OUTPUT "${MAVLINK_ENUMS_H}" "${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}"
    COMMAND Python3::Interpreter "${MAVLINK_ENUMS_GENERATOR}" "${QGC_MAVLINK_GENERATED_DIR}"
            "${MAVLINK_ENUMS_H}" "${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}"
    DEPENDS mavlink "${MAVLINK_ENUMS_GENERATOR}" "${CMAKE_SOURCE_DIR}/tools/common/io.py"
    COMMENT "Generating MAVLinkEnums.h + MAVLinkEnumsQml.{h,cc}"
    VERBATIM
)
add_custom_target(mavlink_enums DEPENDS "${MAVLINK_ENUMS_H}" "${MAVLINK_ENUMS_QML_H}" "${MAVLINK_ENUMS_QML_CC}")
add_dependencies(${CMAKE_PROJECT_NAME} mavlink_enums)
foreach(_mavlink_provider_consumer IN LISTS QGC_MAVLINK_PROVIDER_CONSUMERS)
    if(NOT TARGET ${_mavlink_provider_consumer})
        message(FATAL_ERROR "QGC: MAVLink provider consumer target does not exist: ${_mavlink_provider_consumer}")
    endif()
    add_dependencies(${_mavlink_provider_consumer} mavlink_enums)
endforeach()
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
add_custom_command(
    OUTPUT "${MAVLINK_INSTANCE_FIELDS_H}"
    COMMAND Python3::Interpreter "${MAVLINK_INSTANCE_FIELDS_GENERATOR}"
            "${QGC_MAVLINK_XML_DIR}" "${QGC_MAVLINK_DIALECT}" "${MAVLINK_INSTANCE_FIELDS_H}"
    DEPENDS mavlink "${MAVLINK_INSTANCE_FIELDS_GENERATOR}"
            "${CMAKE_SOURCE_DIR}/tools/common/io.py" "${CMAKE_SOURCE_DIR}/tools/common/xml.py"
    COMMENT "Generating MAVLinkInstanceFields.h"
    VERBATIM
)
add_custom_target(mavlink_instance_fields DEPENDS "${MAVLINK_INSTANCE_FIELDS_H}")
add_dependencies(${CMAKE_PROJECT_NAME} mavlink_instance_fields)

set(MAVLINK_SCHEMA_FILES
    "${QGC_MAVLINK_COMPONENT_METADATA_DIR}/parameter.schema.json"
    "${QGC_MAVLINK_COMPONENT_METADATA_DIR}/general.schema.json"
    "${QGC_MAVLINK_COMPONENT_METADATA_DIR}/actuators.schema.json"
    "${QGC_MAVLINK_COMPONENT_METADATA_DIR}/peripherals.schema.json"
    "${QGC_MAVLINK_COMPONENT_METADATA_DIR}/translation.schema.json"
)
qt_add_resources(${CMAKE_PROJECT_NAME} mavlink_component_metadata_schemas
    PREFIX "/json/component_metadata"
    BASE "${QGC_MAVLINK_COMPONENT_METADATA_DIR}"
    FILES ${MAVLINK_SCHEMA_FILES}
)

# ----------------------------------------------------------------------------
# LibEvents - MAVLink event handling
# ----------------------------------------------------------------------------
add_subdirectory(LibEvents)
add_subdirectory(Signing)
