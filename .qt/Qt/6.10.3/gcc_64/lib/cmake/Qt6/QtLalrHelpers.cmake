# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Match the pattern 'regex' in 'input_line', replace the match with 'replacement'
# and set that result in 'out_var' in the parent scope.
function(qt_regex_match_and_get input_line regex replacement out_var)
    string(REGEX MATCH "${regex}" match "${input_line}")
    if(match)
        string(REGEX REPLACE "${regex}" "${replacement}" match "${input_line}")
        string(STRIP ${match} match)
        set(${out_var} "${match}" PARENT_SCOPE)
    endif()
endfunction()

# Match 'regex' in a list of lines. When found, set the value to 'out_var' and break early.
function(qt_qlalr_find_option_in_list input_list regex out_var)
    foreach(line ${input_list})
        qt_regex_match_and_get("${line}" "${regex}" "\\1" option)
        if(option)
            string(TOLOWER ${option} option)
            set(${out_var} "${option}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    message(FATAL_ERROR "qt_qlalr_find_option_in_list: Could not extract ${out_var}")
endfunction()

# Generate a few output files using qlalr, and assign those to 'consuming_target'.
# 'input_file_list' is a list of 'foo.g' file paths.
# 'flags' are extra flags to be passed to qlalr.
function(qt_process_qlalr consuming_target input_file_list flags)
    # Don't try to extend_target when cross compiling an imported host target (like a tool).
    qt_is_imported_target("${consuming_target}" is_imported)
    if(is_imported)
        return()
    endif()

    qt_internal_is_skipped_test(skipped ${consuming_target})
    if(skipped)
        return()
    endif()
    qt_internal_is_in_test_batch(in_batch ${consuming_target})
    if(in_batch)
        _qt_internal_test_batch_target_name(consuming_target)
    endif()

    foreach(input_file ${input_file_list})
        file(STRINGS ${input_file} input_file_lines)
        qt_qlalr_find_option_in_list("${input_file_lines}" "^%parser(.+)" "parser")
        qt_qlalr_find_option_in_list("${input_file_lines}" "^%decl(.+)" "decl")
        qt_qlalr_find_option_in_list("${input_file_lines}" "^%impl(.+)" "impl")
        get_filename_component(base_file_name ${input_file} NAME_WE)

        # Pass a relative input file path to qlalr to generate relative #line directives.
        if(IS_ABSOLUTE "${input_file}")
            set(absolute_input_file "${input_file}")
        else()
            get_filename_component(absolute_input_file "${input_file}" ABSOLUTE)
        endif()
        file(RELATIVE_PATH relative_input_file "${CMAKE_CURRENT_BINARY_DIR}"
            "${absolute_input_file}")

        set(cpp_file "${parser}.cpp")
        set(private_file "${CMAKE_CURRENT_BINARY_DIR}/${parser}_p.h")
        set(decl_file "${CMAKE_CURRENT_BINARY_DIR}/${decl}")
        set(impl_file "${impl}")
        add_custom_command(
            OUTPUT ${cpp_file} ${private_file} ${decl_file} ${impl_file}
            COMMAND ${QT_CMAKE_EXPORT_NAMESPACE}::qlalr ${flags} ${relative_input_file}
            DEPENDS ${QT_CMAKE_EXPORT_NAMESPACE}::qlalr
            MAIN_DEPENDENCY ${input_file}
            VERBATIM
        )
        target_sources(${consuming_target} PRIVATE ${cpp_file} ${impl_file}
            ${private_file} ${decl_file})
    endforeach()
endfunction()
