include_guard(GLOBAL)

set(_synclair_message_definitions_source_dir "${CMAKE_SOURCE_DIR}/message-definitions")
set(_synclair_message_definitions_binary_dir "${CMAKE_BINARY_DIR}/SynclairVision/message-definitions")

if(NOT EXISTS "${_synclair_message_definitions_source_dir}/CMakeLists.txt"
   OR NOT EXISTS "${_synclair_message_definitions_source_dir}/mavlink/message_definitions/v1.0/all.xml"
)
    message(FATAL_ERROR
        "Synclair: Recursive message-definitions submodules are missing. "
        "Run 'git submodule update --init --recursive' or 'just submodules', then configure again."
    )
endif()

set(DIGIVIEW_MAVLINK_WIRE_PROTOCOL "${QGC_MAVLINK_VERSION}" CACHE STRING
    "MAVLink wire protocol version used for generated C headers" FORCE
)
add_subdirectory(
    "${_synclair_message_definitions_source_dir}"
    "${_synclair_message_definitions_binary_dir}"
)

if(NOT TARGET DigiView::MAVLinkHeaders)
    message(FATAL_ERROR "Synclair: message-definitions did not provide DigiView::MAVLinkHeaders")
endif()

get_target_property(_synclair_generated_include_root
    DigiView::MAVLinkHeaders INTERFACE_INCLUDE_DIRECTORIES
)
list(LENGTH _synclair_generated_include_root _synclair_generated_include_root_count)
if(NOT _synclair_generated_include_root_count EQUAL 1)
    message(FATAL_ERROR
        "Synclair: DigiView::MAVLinkHeaders must provide exactly one generated include root; "
        "found ${_synclair_generated_include_root_count}"
    )
endif()

set(QGC_MAVLINK_GENERATED_DIR
    "${_synclair_generated_include_root}/mavlink/v${QGC_MAVLINK_VERSION}"
)
set(QGC_MAVLINK_XML_DIR
    "${_synclair_message_definitions_binary_dir}/mavlink/message_definitions/v1.0"
)
set(QGC_MAVLINK_COMPONENT_METADATA_DIR
    "${_synclair_message_definitions_source_dir}/mavlink/component_metadata"
)

add_library(mavlink INTERFACE)
add_dependencies(mavlink digiview_mavlink_generate_headers)
target_include_directories(mavlink INTERFACE
    "${QGC_MAVLINK_GENERATED_DIR}"
    "${QGC_MAVLINK_GENERATED_DIR}/${QGC_MAVLINK_DIALECT}"
)
