# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Add a custom ${module_target}_headersclean_check target that builds each header in
# ${module_headers} with a custom set of defines. This makes sure our public headers
# are self-contained, and also compile with more strict compiler options.
function(qt_internal_add_headersclean_target module_target module_headers)
    if(INPUT_headersclean AND WASM)
        message(FATAL_ERROR "The headersclean targets are not supported on WASM platform.")
    endif()

    get_target_property(no_headersclean_check ${module_target} _qt_no_headersclean_check)
    if(no_headersclean_check)
        return()
    endif()

    set(hclean_headers "")
    foreach(header IN LISTS module_headers)
        get_filename_component(header_name "${header}" NAME)
        if(header_name MATCHES "^q[^_]+\\.h$" AND NOT header_name MATCHES ".*(global|exports)\\.h")
            list(APPEND hclean_headers "${header}")
        endif()
    endforeach()

    # Make sure that the header compiles with our strict options
    set(hcleanDEFS -DQT_NO_CAST_TO_ASCII
                 -DQT_NO_CAST_FROM_ASCII
                 -DQT_NO_URL_CAST_FROM_STRING
                 -DQT_NO_CAST_FROM_BYTEARRAY
                 -DQT_NO_CONTEXTLESS_CONNECT
                 -DQT_NO_KEYWORDS
                 -DQT_TYPESAFE_FLAGS
                 -DQT_USE_QSTRINGBUILDER
                 -DQT_USE_FAST_OPERATOR_PLUS)

    set(compiler_to_run "${CMAKE_CXX_COMPILER}")
    if(CMAKE_CXX_COMPILER_LAUNCHER)
        list(PREPEND compiler_to_run "${CMAKE_CXX_COMPILER_LAUNCHER}")
    endif()

    set(prop_prefix "")
    get_target_property(target_type "${module_target}" TYPE)
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(prop_prefix "INTERFACE_")
    endif()

    set(target_includes_genex
        "$<TARGET_PROPERTY:${module_target},${prop_prefix}INCLUDE_DIRECTORIES>")
    set(includes_exist_genex "$<BOOL:${target_includes_genex}>")
    set(target_includes_joined_genex
        "$<${includes_exist_genex}:-I$<JOIN:${target_includes_genex},;-I>>")

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        list(GET CMAKE_CONFIGURATION_TYPES 0 first_config_type)
        set(config_suffix "$<$<NOT:$<CONFIG:${first_config_type}>>:-$<CONFIG>>")
    endif()

    # qmake doesn't seem to add the defines that are set by the header_only_module when checking the
    # the cleanliness of the module's header files.
    # This allows us to bypass an error with CMake 3.18 and lower when trying to evaluate
    # genex-enhanced compile definitions. An example of that is in
    # qttools/src/designer/src/uiplugin/CMakeLists.txt which ends up causing the following error
    # message:
    #  CMake Error at qtbase/cmake/QtModuleHelpers.cmake:35 (add_library):
    #    INTERFACE_LIBRARY targets may only have whitelisted properties.  The
    #    property "QT_PLUGIN_CLASS_NAME" is not allowed.
    #  Call Stack (most recent call first):
    #    src/designer/src/uiplugin/CMakeLists.txt:7 (qt_internal_add_module)
    if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
        set(target_defines_genex
            "$<TARGET_PROPERTY:${module_target},${prop_prefix}COMPILE_DEFINITIONS>")
        set(defines_exist_genex "$<BOOL:${target_defines_genex}>")
        set(target_defines_joined_genex
            "$<${defines_exist_genex}:-D$<JOIN:${target_defines_genex},;-D>>")
    endif()

    # TODO: FIXME
    # Passing COMPILE_OPTIONS can break add_custom_command() if the values contain genexes
    # that add_custom_command does not support.
    #
    # Such a case happens on Linux where libraries linking against Threads::Threads bring in a
    # '<$<COMPILE_LANG_AND_ID:CUDA,NVIDIA>' genex.
    #
    # If this is really required for headersclean (and qmake's headerclean implementation does
    # actually pass all flags of the associated target), we'll have to replace the genex usage
    # with an implementation that recursively processes a target's dependencies property
    # to compile an expanded list of values for COMPILE_OPTIONS, and then filter out any genexes.
    #
    # This is similar to the proposed workaround at
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21074#note_814979
    #
    # See also https://gitlab.kitware.com/cmake/cmake/-/issues/21336
    #
    #set(target_compile_options_genex "$<TARGET_PROPERTY:${module_target},COMPILE_OPTIONS>")
    #set(compile_options_exist_genex "$<BOOL:${target_compile_options_genex}>")
    #set(target_compile_options_joined_genex
    #    "$<${compile_options_exist_genex}:$<JOIN:${target_compile_options_genex},;>>")

    set(target_compile_flags_genex
        "$<TARGET_PROPERTY:${module_target},${prop_prefix}COMPILE_FLAGS>")
    set(compile_flags_exist_genex "$<BOOL:${target_compile_flags_genex}>")
    set(target_compile_flags_joined_genex
        "$<${compile_flags_exist_genex}:$<JOIN:${target_compile_flags_genex},;>>")

    if (QT_FEATURE_cxx2c)
        set(QT_HEADERS_CLEAN_CXX_STANDARD c++2c)
    elseif (QT_FEATURE_cxx2b)
        set(QT_HEADERS_CLEAN_CXX_STANDARD c++2b)
    else()
        set(QT_HEADERS_CLEAN_CXX_STANDARD c++20)
    endif()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
            OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang|IntelLLVM")

        # Compile header in strict C++20 (or later) mode for C++17 build. Enable further warnings.
        set(hcleanFLAGS -std=${QT_HEADERS_CLEAN_CXX_STANDARD}
            -Wall -Wextra -Werror -pedantic-errors
            -Woverloaded-virtual -Wshadow -Wundef -Wfloat-equal
            -Wnon-virtual-dtor -Wpointer-arith -Wformat-security
            -Wchar-subscripts -Wold-style-cast
            -Wredundant-decls # QTBUG-115583
            -fno-operator-names)

        if(QT_FEATURE_reduce_relocations AND UNIX)
            list(APPEND hcleanFLAGS -fPIC)
        endif()

        if (NOT ((TEST_architecture_arch STREQUAL arm)
                OR (TEST_architecture_arch STREQUAL mips)))
            list(APPEND hcleanFLAGS -Wcast-align)
        endif()

        if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
            list(APPEND hcleanFLAGS -Wzero-as-null-pointer-constant
                -Wdouble-promotion -Wfloat-conversion)
        endif()

        if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang|IntelLLVM")
            list(APPEND hcleanFLAGS -Wshorten-64-to-32
                -Wweak-vtables)
        endif()

        separate_arguments(cxx_flags NATIVE_COMMAND ${CMAKE_CXX_FLAGS})

        if(APPLE AND CMAKE_OSX_SYSROOT)
            list(APPEND cxx_flags "${CMAKE_CXX_SYSROOT_FLAG}" "${CMAKE_OSX_SYSROOT}")
        endif()

        if(APPLE AND QT_FEATURE_framework)
            # For some reason CMake doesn't generate -iframework flags from the INCLUDE_DIRECTORIES
            # generator expression we provide, so pass it explicitly and hope for the best.
            list(APPEND framework_includes
                 "-iframework" "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBDIR}")

            # If additional package prefixes are provided, we consider they can contain frameworks
            # as well.
            foreach(prefix IN LISTS _qt_additional_packages_prefix_paths)
                __qt_internal_reverse_prefix_path_from_cmake_dir(path "${path}")

                set(libdir "${prefix}/${INSTALL_LIBDIR}")
                if(EXISTS "${libdir}")
                    list(APPEND framework_includes
                        "-iframework" "${libdir}")
                endif()
            endforeach()
        endif()

        set(compiler_command_line
            "${compiler_to_run}" "-c" "${cxx_flags}"
            "${target_compile_flags_joined_genex}"
            "${target_defines_joined_genex}"
            "${hcleanFLAGS}"
            "${target_includes_joined_genex}"
            "${framework_includes}"
            "${hcleanDEFS}"

        )
        string(JOIN " " compiler_command_line_variables
            "-xc++"
            "\${INPUT_HEADER_FILE}"
            "-o"
            "\${OUTPUT_ARTIFACT}"
        )
        set(input_header_path_type ABSOLUTE)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        # Note we can't enable -Za, as it does not support certain key Microsoft SDK header files
        # we use. Microsoft suggests to use /permissive- instead, which is implicity set by
        # -std:c++latest.
        set(hcleanFLAGS -std:c++latest -Zc:__cplusplus -WX -W4 -EHsc)

        # Because we now add `-DNOMINMAX` to `PlatformCommonInternal`.
        set(hcleanUDEFS -UNOMINMAX)

        # cl.exe needs a source path
        get_filename_component(source_path "${QT_MKSPECS_DIR}/features/data/dummy.cpp" REALPATH)

        set(compiler_command_line
            "${compiler_to_run}" "-nologo" "-c" "${CMAKE_CXX_FLAGS}"
            "${target_compile_flags_joined_genex}"
            "${target_defines_joined_genex}"
            "${hcleanFLAGS}"
            "${target_includes_joined_genex}"
            "${hcleanDEFS}"
            "${hcleanUDEFS}"
        )
        string(JOIN " " compiler_command_line_variables
            "-FI"
            "\${INPUT_HEADER_FILE}"
            "-Fo\${OUTPUT_ARTIFACT}"
            "${source_path}"
        )

        set(input_header_path_type REALPATH)
    else()
        message(FATAL_ERROR "CMAKE_CXX_COMPILER_ID \"${CMAKE_CXX_COMPILER_ID}\" is not supported"
            " for the headersclean check.")
    endif()

    qt_internal_module_info(module ${module_target})

    unset(header_check_exceptions)
    set(header_check_exceptions
        "${CMAKE_CURRENT_BINARY_DIR}/${module}_header_check_exceptions")
    set(headers_check_parameters
        "${CMAKE_CURRENT_BINARY_DIR}/${module_target}HeadersCheckParameters${config_suffix}.cmake")
    string(JOIN "\n" headers_check_parameters_content
        "set(HEADER_CHECK_EXCEPTIONS"
        "    \"${header_check_exceptions}\")"
        "set(HEADER_CHECK_COMPILER_COMMAND_LINE"
        "    \[\[$<JOIN:${compiler_command_line},\]\]\n    \[\[>\]\]\n"
        "    ${compiler_command_line_variables}"
        ")"
    )
    file(GENERATE OUTPUT "${headers_check_parameters}"
        CONTENT "${headers_check_parameters_content}")

    set(sync_headers_dep "${module_target}_sync_headers")

    foreach(header ${hclean_headers})
        # We need realpath here to make sure path starts with drive letter
        get_filename_component(input_path "${header}" ${input_header_path_type})

        get_filename_component(input_file_name ${input_path} NAME)
        set(artifact_path "${CMAKE_CURRENT_BINARY_DIR}/header_check/${input_file_name}.o")

        set(possible_base_dirs "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}")
        foreach(dir IN LISTS possible_base_dirs)
            _qt_internal_path_is_prefix(dir "${input_path}" dir_is_prefix)
            if(dir_is_prefix)
                set(input_base_dir "${dir}")
                break()
            endif()
        endforeach()

        if(input_base_dir AND IS_ABSOLUTE "${input_base_dir}" AND IS_ABSOLUTE "${input_path}")
            file(RELATIVE_PATH comment_header_path "${input_base_dir}" "${input_path}")
        else()
            set(comment_header_path "${input_path}")
        endif()

        add_custom_command(
            OUTPUT "${artifact_path}"
            COMMENT "headersclean: Checking header ${comment_header_path}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/header_check"
            COMMAND ${CMAKE_COMMAND}
                -DINPUT_HEADER_FILE=${input_path}
                -DOUTPUT_ARTIFACT=${artifact_path}
                -DPARAMETERS=${headers_check_parameters}
                -P "${QT_CMAKE_DIR}/QtModuleHeadersCheck.cmake"
            IMPLICIT_DEPENDS CXX
            VERBATIM
            COMMAND_EXPAND_LISTS
            DEPENDS
                ${headers_check_parameters}
                ${sync_headers_dep}
                ${input_path}
                ${header_check_exceptions}
        )
        list(APPEND hclean_artifacts "${artifact_path}")
    endforeach()

    add_custom_target(${module_target}_headersclean_check
        COMMENT "headersclean: Checking headers in ${module}"
        DEPENDS ${hclean_artifacts}
        VERBATIM)

    if(NOT TARGET headersclean_check)
        add_custom_target(headersclean_check ALL)
    endif()

    add_dependencies(headersclean_check ${module_target}_headersclean_check)
endfunction()
