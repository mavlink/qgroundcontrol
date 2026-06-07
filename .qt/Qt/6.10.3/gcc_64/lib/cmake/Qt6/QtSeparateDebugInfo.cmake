# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(CMAKE_VERSION VERSION_LESS 3.17.0)
set(CMAKE_CURRENT_FUNCTION_LIST_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

# Builds a shared library which will have strip run on it.
function(qt_internal_try_compile_binary_for_strip binary_out_var)
    # Need to find the config.tests files depending whether the qtbase sources are available.
    # This mirrors the logic in qt_set_up_build_internals_paths.
    # TODO: Clean this up, together with qt_set_up_build_internals_paths to only use the
    # the qtbase sources when building qtbase. And perhaps also when doing a non-prefix
    # developer-build.

    set(config_test_dir "config.tests/binary_for_strip")
    set(qtbase_config_test_dir "${QT_SOURCE_TREE}/${config_test_dir}")
    set(installed_config_test_dir
        "${_qt_cmake_dir}/${QT_CMAKE_EXPORT_NAMESPACE}/${config_test_dir}")

    # qtbase sources available, always use them, regardless of prefix or non-prefix builds.
    if(EXISTS "${qtbase_config_test_dir}")
        set(src_dir "${qtbase_config_test_dir}")

    # qtbase sources unavailable, use installed files.
    elseif(EXISTS "${installed_config_test_dir}")
        set(src_dir "${installed_config_test_dir}")
    else()
        message(FATAL_ERROR "Can't find binary_for_strip config test project.")
    endif()

    # Make sure the built project files are not installed when doing an in-source build (like it
    # happens in Qt's CI) by choosing a build dir that does not coincide with the installed
    # source dir. Otherwise the config test binaries will be packaged up, which we don't want.
    set(binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${config_test_dir}_built")

    set(flags "")
    qt_get_platform_try_compile_vars(platform_try_compile_vars)
    list(APPEND flags ${platform_try_compile_vars})

    # CI passes the project dir of the Qt repository as absolute path without drive letter:
    #   \Users\qt\work\qt\qtbase
    # Ensure that arg_PROJECT_PATH is an absolute path with drive letter:
    #   C:/Users/qt/work/qt/qtbase
    # This works around CMake upstream issue #22534.
    if(CMAKE_HOST_WIN32)
        get_filename_component(src_dir "${src_dir}" REALPATH)
    endif()

    # Build a real binary that strip can be run on.
    try_compile(QT_INTERNAL_BUILT_BINARY_FOR_STRIP
        "${binary_dir}"
        "${src_dir}"
        binary_for_strip # project name
        OUTPUT_VARIABLE build_output
        CMAKE_FLAGS ${flags}
    )

    # Retrieve the binary path from the build output.
    string(REGEX REPLACE ".+###(.+)###.+" "\\1" output_binary_path "${build_output}")

    if(NOT EXISTS "${output_binary_path}")
        message(FATAL_ERROR "Extracted binary path for strip does not exist: ${output_binary_path}")
    endif()

    set(${binary_out_var} "${output_binary_path}" PARENT_SCOPE)
endfunction()

# Helper to defer calling the separate debug info helper args until the finalizer is run.
function(qt_internal_defer_separate_debug_info target)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        SEPARATE_DEBUG_INFO_ARGS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set_property(TARGET "${target}" PROPERTY _qt_finalize_separate_debug_info_enabled TRUE)
    set_property(TARGET "${target}" APPEND PROPERTY _qt_finalize_separate_debug_info_args
        ${arg_SEPARATE_DEBUG_INFO_ARGS}
    )
endfunction()

# Handler to run the separate debug info helper using finalizer args.
function(qt_internal_finalize_executable_separate_debug_info target)
    get_target_property(enabled "${target}" _qt_finalize_separate_debug_info_enabled)
    if(NOT enabled)
        return()
    endif()

    get_target_property(args "${target}" _qt_finalize_separate_debug_info_args)
    if(NOT args)
        set(args "")
    endif()

    qt_enable_separate_debug_info("${target}" ${args})
endfunction()

# When using the MinGW 11.2.0 toolchain, cmake --install --strip as used by
# qt-cmake-private-install.cmake, removes the .gnu_debuglink section in binaries and thus
# breaks the separate debug info feature.
#
# Generate a wrapper shell script that passes an option to keep the debug section.
# The wrapper is used when targeting Linux or MinGW with a shared Qt build.
# The check to see if the option is supported by 'strip', is done once for every repo configured,
# because different machines might have different strip versions installed, without support for
# the option we need.
#
# Once CMake supports custom strip arguments, we can remove the part that creates a shell wrapper.
# https://gitlab.kitware.com/cmake/cmake/-/issues/23346
function(qt_internal_generate_binary_strip_wrapper)
    # Return early if check was done already, if explicitly skipped, or when building a static Qt.
    if(DEFINED CACHE{QT_INTERNAL_STRIP_SUPPORTS_KEEP_SECTION}
            OR QT_NO_STRIP_WRAPPER
            OR (NOT QT_BUILD_SHARED_LIBS)
        )
        return()
    endif()

    # Backup the original strip path on very first configuration call.
    # The value might have been determined by CMake via CMakeDetermineCXXCompiler ->
    # CMakeFindBinUtils -> find_program(), or it might have been set by a toolchain file.
    if(NOT QT_INTERNAL_ORIGINAL_CMAKE_STRIP AND CMAKE_STRIP)
        set(QT_INTERNAL_ORIGINAL_CMAKE_STRIP "${CMAKE_STRIP}" CACHE INTERNAL
            "Original strip binary")
    endif()

    message(STATUS "CMAKE_STRIP (original): ${QT_INTERNAL_ORIGINAL_CMAKE_STRIP}")

    # Target Linux and MinGW.
    if((UNIX OR MINGW)
            AND NOT APPLE
            AND NOT ANDROID
            AND CMAKE_STRIP)

        # To make reconfiguration more robust when QT_INTERNAL_STRIP_SUPPORTS_KEEP_SECTION is
        # manually removed, make sure to always restore the original strip first, by
        # re-assigning the original value.
        set(CMAKE_STRIP "${QT_INTERNAL_ORIGINAL_CMAKE_STRIP}" CACHE STRING "")

        # Getting path to a binary we can run strip on.
        qt_internal_try_compile_binary_for_strip(valid_binary_path)

        # The strip arguments are used both for the execute_process test and also as content
        # in the file created by configure_file.
        set(strip_arguments "--keep-section=.gnu_debuglink")

        # Check if the option is supported.
        message(STATUS "Performing Test strip --keep-section")
        execute_process(
            COMMAND
                "${CMAKE_STRIP}" ${strip_arguments} "${valid_binary_path}"
            OUTPUT_VARIABLE strip_probe_output
            ERROR_VARIABLE strip_probe_output
            RESULT_VARIABLE strip_result_var
        )

        # A successful strip of a binary should have a '0' exit code.
        if(NOT strip_result_var STREQUAL "0")
            set(keep_section_supported FALSE)
        else()
            set(keep_section_supported TRUE)
        endif()

        # Cache the result.
        set(QT_INTERNAL_STRIP_SUPPORTS_KEEP_SECTION "${keep_section_supported}" CACHE BOOL
            "strip supports --keep-section")

        message(DEBUG
            "qt_internal_generate_binary_strip_wrapper:\n"
            "original strip: ${QT_INTERNAL_ORIGINAL_CMAKE_STRIP}\n"
            "strip probe output: ${strip_probe_output}\n"
            "strip result: ${strip_result_var}\n"
            "keep section supported: ${keep_section_supported}\n"
        )
        message(STATUS "Performing Test strip --keep-section - ${keep_section_supported}")

        # If the option is not supported, don't generate a wrapper and just use the stock binary.
        if(NOT keep_section_supported)
            return()
        endif()

        set(wrapper_extension "")

        if(NOT CMAKE_HOST_UNIX)
            set(wrapper_extension ".bat")
        endif()

        set(script_name "qt-internal-strip")

        # the libexec literal is used on purpose for the source, so the file is found
        # on Windows hosts.
        set(wrapper_rel_path "libexec/${script_name}${wrapper_extension}.in")

        # Need to find the libexec input file depending whether the qtbase sources are available.
        # This mirrors the logic in qt_set_up_build_internals_paths.
        # TODO: Clean this up, together with qt_set_up_build_internals_paths to only use the
        # the qtbase sources when building qtbase. And perhaps also when doing a non-prefix
        # developer-build.
        set(qtbase_wrapper_in_path "${QT_SOURCE_TREE}/${wrapper_rel_path}")
        set(installed_wrapper_in_path
            "${_qt_cmake_dir}/${QT_CMAKE_EXPORT_NAMESPACE}/${wrapper_rel_path}")

        # qtbase sources available, always use them, regardless of prefix or non-prefix builds.
        if(EXISTS "${qtbase_wrapper_in_path}")
            set(wrapper_in "${qtbase_wrapper_in_path}")

        # qtbase sources unavailable, use installed files.
        elseif(EXISTS "${installed_wrapper_in_path}")
            set(wrapper_in "${installed_wrapper_in_path}")
        else()
            message(FATAL_ERROR "Can't find ${script_name}${wrapper_extension}.in file.")
        endif()

        set(wrapper_out "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/${script_name}${wrapper_extension}")

        # Used in the template file.
        set(original_strip "${QT_INTERNAL_ORIGINAL_CMAKE_STRIP}")

        configure_file("${wrapper_in}" "${wrapper_out}" @ONLY)

        # Override the strip binary to be used by CMake install target.
        set(CMAKE_STRIP "${wrapper_out}" CACHE INTERNAL "Custom Qt strip wrapper")

        message(STATUS "CMAKE_STRIP (used by Qt): ${CMAKE_STRIP}")
    endif()
endfunction()

# Enable separate debug information for the given target
function(qt_enable_separate_debug_info target installDestination)
    set(flags QT_EXECUTABLE)
    if(APPLE)
      set(options DSYM_OUTPUT_DIR)
    else()
      set(options)
    endif()
    set(multiopts ADDITIONAL_INSTALL_ARGS)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    if (NOT QT_FEATURE_separate_debug_info)
        return()
    endif()
    if (NOT UNIX AND NOT MINGW)
        return()
    endif()
    get_target_property(target_type ${target} TYPE)
    if (NOT target_type STREQUAL "MODULE_LIBRARY" AND
        NOT target_type STREQUAL "SHARED_LIBRARY" AND
        NOT target_type STREQUAL "EXECUTABLE")
        return()
    endif()
    get_property(target_source_dir TARGET ${target} PROPERTY SOURCE_DIR)
    get_property(skip_separate_debug_info DIRECTORY "${target_source_dir}" PROPERTY _qt_skip_separate_debug_info)
    if (skip_separate_debug_info)
        return()
    endif()

    unset(commands)
    if(APPLE)
        find_program(DSYMUTIL_PROGRAM dsymutil)
        set(copy_bin ${DSYMUTIL_PROGRAM})
        set(strip_bin ${CMAKE_STRIP})
        set(debug_info_suffix dSYM)
        set(copy_bin_out_arg --flat -o)
        set(strip_args -S)
    else()
        set(copy_bin ${CMAKE_OBJCOPY})
        set(strip_bin ${CMAKE_OBJCOPY})
        if(QNX)
            set(debug_info_suffix sym)
            set(debug_info_keep --keep-file-symbols)
            set(strip_args --strip-debug -R.ident)
        else()
            set(debug_info_suffix debug)
            set(debug_info_keep --only-keep-debug)
            set(strip_args --strip-debug)
        endif()
    endif()
    if(APPLE)
        get_target_property(is_framework ${target} FRAMEWORK)
        if(is_framework)
            qt_internal_get_framework_info(fw ${target})
            set(BUNDLE_ID ${fw_name})
        else()
            set(BUNDLE_ID ${target})
        endif()

        get_target_property(is_bundle ${target} MACOSX_BUNDLE)

        if (NOT "x${arg_DSYM_OUTPUT_DIR}" STREQUAL "x")
            set(debug_info_bundle_dir "${arg_DSYM_OUTPUT_DIR}/${target}")
        elseif(is_framework OR is_bundle)
            set(debug_info_bundle_dir "$<TARGET_BUNDLE_DIR:${target}>")
        else()
            set(debug_info_bundle_dir "$<TARGET_FILE:${target}>")
        endif()
        set(debug_info_bundle_dir "${debug_info_bundle_dir}.${debug_info_suffix}")

        set(debug_info_contents_dir "${debug_info_bundle_dir}/Contents")
        set(debug_info_target_dir "${debug_info_contents_dir}/Resources/DWARF")
        configure_file(
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/QtSeparateDebugInfo.Info.plist.in"
            "Info.dSYM.plist"
            )
        list(APPEND commands
            COMMAND ${CMAKE_COMMAND} -E make_directory ${debug_info_target_dir}
            COMMAND ${CMAKE_COMMAND} -E copy "Info.dSYM.plist" "${debug_info_contents_dir}/Info.plist"
            )
        set(debug_info_target "${debug_info_target_dir}/$<TARGET_FILE_BASE_NAME:${target}>")

        if(arg_QT_EXECUTABLE AND QT_FEATURE_debug_and_release)
            qt_get_cmake_configurations(cmake_configs)
            foreach(cmake_config ${cmake_configs})
                # Make installation optional for targets that are not built by default in this config
                if(NOT (cmake_config STREQUAL QT_MULTI_CONFIG_FIRST_CONFIG))
                    set(install_optional_arg OPTIONAL)
                  else()
                    unset(install_optional_arg)
                endif()
                qt_install(DIRECTORY ${debug_info_bundle_dir}
                           ${arg_ADDITIONAL_INSTALL_ARGS}
                           ${install_optional_arg}
                           CONFIGURATIONS ${cmake_config}
                           DESTINATION ${installDestination})
            endforeach()
        else()
            qt_install(DIRECTORY ${debug_info_bundle_dir}
                       ${arg_ADDITIONAL_INSTALL_ARGS}
                       DESTINATION ${installDestination})
        endif()
    else()
        set(debug_info_target "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.${debug_info_suffix}")
        qt_install(FILES ${debug_info_target} DESTINATION ${installDestination})
    endif()
    list(APPEND commands
        COMMAND ${copy_bin} ${debug_info_keep} $<TARGET_FILE:${target}>
                ${copy_bin_out_arg} ${debug_info_target}
        COMMAND ${strip_bin} ${strip_args} $<TARGET_FILE:${target}>
        )
    if(NOT APPLE)
        list(APPEND commands
            COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=${debug_info_target} $<TARGET_FILE:${target}>
            )
    endif()
    if(NOT CMAKE_HOST_WIN32)
        list(APPEND commands
            COMMAND chmod -x ${debug_info_target}
            )
    endif()
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        ${commands}
        VERBATIM)
endfunction()
