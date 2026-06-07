# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt6_generate_wayland_protocol_server_sources target)
    cmake_parse_arguments(arg "PUBLIC_CODE;PRIVATE_CODE" "__QT_INTERNAL_WAYLAND_INCLUDE_DIR" "FILES" ${ARGN})
    if(DEFINED arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments were passed to qt6_generate_wayland_protocol_server_sources: (${arg_UNPARSED_ARGUMENTS}).")
    endif()

    get_target_property(target_binary_dir ${target} BINARY_DIR)

    if(NOT TARGET Wayland::Scanner)
        message(FATAL_ERROR "Wayland::Scanner target not found. You might be missing the WaylandScanner CMake package.")
    endif()

    if(NOT TARGET Qt6::qtwaylandscanner)
        message(FATAL_ERROR "qtwaylandscanner executable not found. Most likely there is an issue with your Qt installation.")
    endif()

    string(TOUPPER "${target}" module_define_infix)
    string(REPLACE "-" "_" module_define_infix "${module_define_infix}")
    string(REPLACE "." "_" module_define_infix "${module_define_infix}")
    set(build_macro "QT_BUILD_${module_define_infix}_LIB")

    if (arg_PRIVATE_CODE)
        set(wayland_scanner_code_option "private-code")
    else()
        set(wayland_scanner_code_option "public-code")
    endif()

    foreach(protocol_file IN LISTS arg_FILES)
        get_filename_component(protocol_name "${protocol_file}" NAME_WLE)

        set(waylandscanner_header_output "${target_binary_dir}/wayland-${protocol_name}-server-protocol.h")
        set(waylandscanner_code_output "${target_binary_dir}/wayland-${protocol_name}-protocol.c")
        set(qtwaylandscanner_header_output "${target_binary_dir}/qwayland-server-${protocol_name}.h")
        set(qtwaylandscanner_code_output "${target_binary_dir}/qwayland-server-${protocol_name}.cpp")

        add_custom_command(
            OUTPUT "${waylandscanner_header_output}"
            #TODO: Maybe put the files in ${CMAKE_CURRENT_BINARY_DIR/wayland_generated instead?
            COMMAND Wayland::Scanner --include-core-only server-header < "${protocol_file}" > "${waylandscanner_header_output}"
            DEPENDS ${protocol_file} Wayland::Scanner
        )
        add_custom_command(
            OUTPUT "${waylandscanner_code_output}"
            COMMAND Wayland::Scanner --include-core-only ${wayland_scanner_code_option} < "${protocol_file}" > "${waylandscanner_code_output}"
            DEPENDS ${protocol_file} Wayland::Scanner
        )

        set(wayland_include_dir "")
        if(arg___QT_INTERNAL_WAYLAND_INCLUDE_DIR)
            set(wayland_include_dir "${arg___QT_INTERNAL_WAYLAND_INCLUDE_DIR}")
        else()
            get_target_property(qt_module ${target} _qt_module_interface_name)
            get_target_property(is_for_module "${target}" _qt_module_has_headers)
            if (qt_module)
                set(wayland_include_dir "Qt${qt_module}/private")
            elseif (is_for_module)
                set(wayland_include_dir "QtWaylandCompositor/private")
            endif()
        endif()

        add_custom_command(
            OUTPUT "${qtwaylandscanner_header_output}"
            COMMAND Qt6::qtwaylandscanner server-header
                "${protocol_file}"
                --build-macro=${build_macro}
                --header-path="${wayland_include_dir}"
                > "${qtwaylandscanner_header_output}"
            DEPENDS ${protocol_file} Qt6::qtwaylandscanner
        )

        add_custom_command(
            OUTPUT "${qtwaylandscanner_code_output}"
            COMMAND Qt6::qtwaylandscanner server-code
                "${protocol_file}"
                --build-macro=${build_macro}
                --header-path="${wayland_include_dir}"
                > "${qtwaylandscanner_code_output}"
            DEPENDS ${protocol_file} Qt6::qtwaylandscanner
        )

        target_sources(${target} PRIVATE
            "${waylandscanner_header_output}"
            "${waylandscanner_code_output}"
            "${qtwaylandscanner_header_output}"
            "${qtwaylandscanner_code_output}"
        )
    endforeach()
    target_include_directories(${target} PRIVATE ${target_binary_dir})
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_generate_wayland_protocol_server_sources)
        qt6_generate_wayland_protocol_server_sources(${ARGV})
    endfunction()
endif()
