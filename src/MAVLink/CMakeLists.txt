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

set(QGC_MESSAGE_DEFINITIONS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/message-definitions")
set(QGC_MESSAGE_DEFINITIONS_BINARY_DIR "${CMAKE_BINARY_DIR}/message-definitions")
if(NOT EXISTS "${QGC_MESSAGE_DEFINITIONS_SOURCE_DIR}/CMakeLists.txt"
   OR NOT EXISTS "${QGC_MESSAGE_DEFINITIONS_SOURCE_DIR}/mavlink/message_definitions/v1.0/all.xml")
    message(FATAL_ERROR
        "QGC: Recursive message-definitions submodules are missing. "
        "Run 'git submodule update --init --recursive' or 'just submodules', then configure again."
    )
endif()

set(DIGIVIEW_MAVLINK_WIRE_PROTOCOL "${QGC_MAVLINK_VERSION}" CACHE STRING
    "MAVLink wire protocol version used for generated C headers" FORCE
)
add_subdirectory("${QGC_MESSAGE_DEFINITIONS_SOURCE_DIR}" "${QGC_MESSAGE_DEFINITIONS_BINARY_DIR}")

if(NOT TARGET DigiView::MAVLinkHeaders)
    message(FATAL_ERROR "QGC: message-definitions did not provide DigiView::MAVLinkHeaders")
endif()

get_target_property(QGC_MAVLINK_GENERATED_INCLUDE_ROOT DigiView::MAVLinkHeaders INTERFACE_INCLUDE_DIRECTORIES)
list(LENGTH QGC_MAVLINK_GENERATED_INCLUDE_ROOT _qgc_mavlink_include_root_count)
if(NOT _qgc_mavlink_include_root_count EQUAL 1)
    message(FATAL_ERROR
        "QGC: DigiView::MAVLinkHeaders must provide exactly one generated include root; "
        "found ${_qgc_mavlink_include_root_count}"
    )
endif()
unset(_qgc_mavlink_include_root_count)

set(QGC_MAVLINK_GENERATED_DIR
    "${QGC_MAVLINK_GENERATED_INCLUDE_ROOT}/mavlink/v${QGC_MAVLINK_VERSION}"
)

# Preserve the MAVLink target and include surface used by QGC while sourcing all
# generated headers from DigiView::MAVLinkHeaders.
add_library(mavlink INTERFACE)
target_link_libraries(mavlink INTERFACE DigiView::MAVLinkHeaders)
add_dependencies(mavlink DigiView::MAVLinkHeaders)
target_include_directories(mavlink INTERFACE
    "${QGC_MAVLINK_GENERATED_DIR}"
    "${QGC_MAVLINK_GENERATED_DIR}/${QGC_MAVLINK_DIALECT}"
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE mavlink)

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
set(MAVLINK_XML_DIR "${QGC_MESSAGE_DEFINITIONS_BINARY_DIR}/mavlink/message_definitions/v1.0")
add_custom_command(
    OUTPUT "${MAVLINK_INSTANCE_FIELDS_H}"
    COMMAND Python3::Interpreter "${MAVLINK_INSTANCE_FIELDS_GENERATOR}"
            "${MAVLINK_XML_DIR}" "${QGC_MAVLINK_DIALECT}" "${MAVLINK_INSTANCE_FIELDS_H}"
    DEPENDS mavlink "${MAVLINK_INSTANCE_FIELDS_GENERATOR}"
            "${CMAKE_SOURCE_DIR}/tools/common/io.py" "${CMAKE_SOURCE_DIR}/tools/common/xml.py"
    COMMENT "Generating MAVLinkInstanceFields.h"
    VERBATIM
)
add_custom_target(mavlink_instance_fields DEPENDS "${MAVLINK_INSTANCE_FIELDS_H}")
add_dependencies(${CMAKE_PROJECT_NAME} mavlink_instance_fields)

# Bundle the upstream component_metadata JSON schemas from the pinned MAVLink submodule.
set(MAVLINK_COMPONENT_METADATA_DIR "${QGC_MESSAGE_DEFINITIONS_SOURCE_DIR}/mavlink/component_metadata")
set(MAVLINK_SCHEMA_FILES
    "${MAVLINK_COMPONENT_METADATA_DIR}/parameter.schema.json"
    "${MAVLINK_COMPONENT_METADATA_DIR}/general.schema.json"
    "${MAVLINK_COMPONENT_METADATA_DIR}/actuators.schema.json"
    "${MAVLINK_COMPONENT_METADATA_DIR}/peripherals.schema.json"
    "${MAVLINK_COMPONENT_METADATA_DIR}/translation.schema.json"
)
qt_add_resources(${CMAKE_PROJECT_NAME} mavlink_component_metadata_schemas
    PREFIX "/json/component_metadata"
    BASE "${MAVLINK_COMPONENT_METADATA_DIR}"
    FILES ${MAVLINK_SCHEMA_FILES}
)

# ----------------------------------------------------------------------------
# LibEvents - MAVLink event handling
# ----------------------------------------------------------------------------
add_subdirectory(LibEvents)
add_subdirectory(Signing)
