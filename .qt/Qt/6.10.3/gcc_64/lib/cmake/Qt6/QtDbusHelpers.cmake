# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# helper to set up a qdbusxml2cpp rule
function(qt_create_qdbusxml2cpp_command target infile)
    cmake_parse_arguments(PARSE_ARGV 2 arg
        "ADAPTOR;INTERFACE"
        "BASENAME"
        "FLAGS")
    _qt_internal_validate_all_args_are_parsed(arg)

    if((arg_ADAPTOR AND arg_INTERFACE) OR (NOT arg_ADAPTOR AND NOT arg_INTERFACE))
        message(FATAL_ERROR "qt_create_dbusxml2cpp_command needs either ADAPTOR or INTERFACE.")
    endif()

    set(option "-a")
    set(type "adaptor")
    if (arg_INTERFACE)
        set(option "-p")
        set(type "interface")
    endif()

    if ("${arg_BASENAME}" STREQUAL "")
        get_filename_component(file_dir "${infile}" DIRECTORY)
        get_filename_component(file_name "${infile}" NAME_WLE)
        get_filename_component(file_ext "${infile}" LAST_EXT)

        if("${file_ext}" STREQUAL ".xml")
        else()
            message(FATAL_ERROR "DBUS ${type} input file is not xml.")
        endif()

        # use last part of io.qt.something.xml!
        get_filename_component(file_ext "${file_name}" LAST_EXT)
        if("x${file_ext}" STREQUAL "x")
        else()
            string(SUBSTRING "${file_ext}" 1 -1 file_name) # cut of leading '.'
        endif()

        string(TOLOWER "${file_name}" file_name)
        set(file_name "${file_name}_${type}")
    else()
        set(file_name ${arg_BASENAME})
    endif()

    # Use absolute file path for the source file and set the current working directory to the
    # current binary directory, because setting an absolute path for the header:source combo option
    # does not work. Splitting on ":" breaks inside the dbus tool when running on Windows
    # due to ":" being contained in the drive path (e.g C:\foo.h:C:\foo.cpp).
    get_filename_component(absolute_in_file_path "${infile}" ABSOLUTE)

    set(header_file "${file_name}.h")
    set(source_file "${file_name}.cpp")

    set(header_file_full "${CMAKE_CURRENT_BINARY_DIR}/${file_name}.h")
    set(source_file_full "${CMAKE_CURRENT_BINARY_DIR}/${file_name}.cpp")

    if(QT_OPTIONAL_TOOLS_PATH)
        if(CMAKE_HOST_WIN32)
            set(executable_suffix ".exe")
        else()
            set(executable_suffix "")
        endif()
        set(tool_path
            "${QT_OPTIONAL_TOOLS_PATH}/${INSTALL_BINDIR}/qdbusxml2cpp${executable_suffix}")
    else()
        set(tool_path "${QT_CMAKE_EXPORT_NAMESPACE}::qdbusxml2cpp")
    endif()

    add_custom_command(OUTPUT "${header_file_full}" "${source_file_full}"
                       COMMAND ${tool_path} ${arg_FLAGS} "${option}"
                               "${header_file}:${source_file}" "${absolute_in_file_path}"
                       DEPENDS "${absolute_in_file_path}" ${tool_path}
                       WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                       COMMAND_EXPAND_LISTS
                       VERBATIM)

    target_sources("${target}" PRIVATE
        "${header_file_full}"
        "${source_file_full}"
    )
    set_source_files_properties(
        "${header_file_full}"
        "${source_file_full}"
        PROPERTIES
            _qt_syncqt_exclude_from_docs TRUE)
endfunction()
