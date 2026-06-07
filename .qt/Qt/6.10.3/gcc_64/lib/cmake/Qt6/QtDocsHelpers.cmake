# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This function adds a dependency between a doc-generating target like 'generate_docs_Gui'
# and the necessary tool target like 'qdoc'.
#
# If the target is not yet existing, save the dependency connection in a global property.
# The dependency is then added near the end of the top-level build after all subdirectories have
# been handled.
function(qt_internal_add_doc_tool_dependency doc_target tool_name)
    qt_get_tool_target_name(tool_target ${tool_name})
    if(TARGET ${tool_target})
        add_dependencies(${doc_target} ${tool_target})
    else()
        qt_internal_defer_dependency(${doc_target} ${tool_target})
    endif()
endfunction()

# Adds custom build and install targets to generate documentation for a documentation project
# identified by a cmake target and a path to a .qdocconf file.
#
# Creates custom targets of the form:
# - generate_docs_${target}
# - prepare_docs_${target}
# - html_docs_${target}
# - install_html_docs_${target}
# - etc.
#
# The first two arguments to the function should be <target> <path-to-qdocconf>.
#
# Additional options are:
# INDEX_DIRECTORIES - a list of index directories to pass to qdoc.
#
# DEFINES - extra environment variable assignments of the form ENV_VAR=VALUE, which should be set
# during qdoc execution.
#
# QDOC_EXTRA_ARGS - extra command-line arguments to pass to qdoc in both prepare and generate
# phases.
#
# QDOC_PREPARE_EXTRA_ARGS - extra command-line arguments to pass to qdoc in the prepare phase.
#
# QDOC_GENERATE_EXTRA_ARGS - extra command-line arguments to pass to qdoc in the generate phase.
#
# SHOW_INTERNAL - if set, the --showinternal option is passed to qdoc.
#
# Additional environment variables considered:
# QT_INSTALL_DOCS - directory path where the qt docs were expected to be installed, used for
# linking to other built docs. If not set, defaults to the qtbase or qt5 build directory, or the
# install directory extracted from the BuildInternals package.
#
# QT_QDOC_EXTRA_ARGS, QT_QDOC_PREPARE_EXTRA_ARGS, QT_QDOC_GENERATE_EXTRA_ARGS - same as the options
# but can be set as either environment or cmake variables.
#
# QT_QDOC_SHOW_INTERNAL - same as the option but can be set as either an environment or
# cmake variable.
function(qt_internal_add_docs)
    if(NOT QT_BUILD_DOCS)
        return()
    endif()

    if(${ARGC} EQUAL 1)
        # Function called from old generated CMakeLists.txt that was missing the target parameter
        if(QT_FEATURE_developer_build)
            message(AUTHOR_WARNING
                "qt_internal_add_docs called with old signature. Skipping doc generation.")
        endif()
        return()
    endif()

    if(NOT ${ARGC} GREATER_EQUAL 2)
        message(FATAL_ERROR
            "qt_internal_add_docs called with a wrong number of arguments. "
            "The call should be qt_internal_add_docs\(<target> <path-to-qdocconf> [other-options])"
        )
    endif()

    set(target ${ARGV0})
    set(qdoc_conf_path ${ARGV1})

    set(opt_args
        SHOW_INTERNAL
        SKIP_JAVADOC
    )
    set(single_args "")
    set(multi_args
        INDEX_DIRECTORIES
        DEFINES
        QDOC_EXTRA_ARGS
        QDOC_PREPARE_EXTRA_ARGS
        QDOC_GENERATE_EXTRA_ARGS
    )
    cmake_parse_arguments(PARSE_ARGV 2 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(qdoc_extra_args "")

    # The INDEX_DIRECTORIES key should enable passing a list of index
    # directories as extra command-line arguments to qdoc, in prepare and
    # generate phases.
    if(arg_INDEX_DIRECTORIES)
        foreach(index_directory ${arg_INDEX_DIRECTORIES})
            list(APPEND qdoc_extra_args "--indexdir" ${index_directory})
        endforeach()
    endif()

    set(show_internal_env FALSE)
    if(DEFINED ENV{QT_QDOC_SHOW_INTERNAL})
        set(show_internal_env $ENV{QT_QDOC_SHOW_INTERNAL})
    endif()
    if(arg_SHOW_INTERNAL OR QT_QDOC_SHOW_INTERNAL OR show_internal_env)
        list(APPEND qdoc_extra_args "--showinternal")
    endif()

    if(arg_QDOC_EXTRA_ARGS)
        list(APPEND qdoc_extra_args ${arg_QDOC_EXTRA_ARGS})
    endif()

    if(QT_QDOC_EXTRA_ARGS)
        list(APPEND qdoc_extra_args ${QT_QDOC_EXTRA_ARGS})
    endif()

    if(DEFINED ENV{QT_QDOC_EXTRA_ARGS})
        list(APPEND qdoc_extra_args $ENV{QT_QDOC_EXTRA_ARGS})
    endif()

    # If a target is not built (which can happen for tools when crosscompiling), we shouldn't try
    # to generate docs.
    if(NOT TARGET "${target}")
        return()
    endif()

    set(tool_dependencies_enabled TRUE)
    if(NOT "${QT_HOST_PATH}" STREQUAL "")
        set(tool_dependencies_enabled FALSE)
        qt_internal_get_host_info_var_prefix(host_info_var_prefix)
        set(doc_tools_bin "${QT_HOST_PATH}/${${host_info_var_prefix}_BINDIR}")
        set(doc_tools_libexec "${QT_HOST_PATH}/${${host_info_var_prefix}_LIBEXECDIR}")
    elseif(NOT "${QT_OPTIONAL_TOOLS_PATH}" STREQUAL "")
        set(tool_dependencies_enabled FALSE)
        set(doc_tools_bin "${QT_OPTIONAL_TOOLS_PATH}/${INSTALL_BINDIR}")
        set(doc_tools_libexec "${QT_OPTIONAL_TOOLS_PATH}/${INSTALL_LIBEXECDIR}")
    elseif(QT_SUPERBUILD)
        set(doc_tools_bin "${QtBase_BINARY_DIR}/${INSTALL_BINDIR}")
        set(doc_tools_libexec "${QtBase_BINARY_DIR}/${INSTALL_LIBEXECDIR}")
    else()
        set(doc_tools_bin "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_BINDIR}")
        set(doc_tools_libexec
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_LIBEXECDIR}")
    endif()

    if(CMAKE_HOST_WIN32)
        set(executable_suffix ".exe")
    else()
        set(executable_suffix "")
    endif()

    set(qdoc_bin "${doc_tools_bin}/qdoc${executable_suffix}")
    set(qtattributionsscanner_bin "${doc_tools_libexec}/qtattributionsscanner${executable_suffix}")
    set(qhelpgenerator_bin "${doc_tools_libexec}/qhelpgenerator${executable_suffix}")

    get_target_property(target_type ${target} TYPE)
    if (NOT target_type STREQUAL "INTERFACE_LIBRARY")
        get_target_property(target_bin_dir ${target} BINARY_DIR)
        get_target_property(target_source_dir ${target} SOURCE_DIR)
    else()
        set(target_bin_dir ${CMAKE_CURRENT_BINARY_DIR})
        set(target_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    set(doc_output_dir "${target_bin_dir}/.doc")

    # Generate include dir list
    set(target_include_dirs_file "${doc_output_dir}/$<CONFIG>/includes.txt")


    set(prop_prefix "")
    if(target_type STREQUAL "INTERFACE_LIBRARY")
        set(prop_prefix "INTERFACE_")
    endif()
    set(include_path_prop "${prop_prefix}INCLUDE_DIRECTORIES")

    set(include_paths_property "$<TARGET_PROPERTY:${target},${include_path_prop}>")
    if (NOT target_type STREQUAL "UTILITY")
        file(GENERATE
            OUTPUT ${target_include_dirs_file}
            CONTENT "$<$<BOOL:${include_paths_property}>:-I$<JOIN:${include_paths_property},\n-I>>"
        )
        set(include_path_args "@${target_include_dirs_file}")
    else()
        set(include_path_args "")
    endif()

    get_filename_component(doc_target "${qdoc_conf_path}" NAME_WLE)
    if (QT_WILL_INSTALL)
        set(qdoc_output_dir "${CMAKE_BINARY_DIR}/${INSTALL_DOCDIR}/${doc_target}")
        set(qdoc_qch_output_dir "${CMAKE_BINARY_DIR}/${INSTALL_DOCDIR}")
        set(index_dir "${CMAKE_BINARY_DIR}/${INSTALL_DOCDIR}")
    else()
        set(qdoc_output_dir
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_DOCDIR}/${doc_target}")
        set(qdoc_qch_output_dir
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_DOCDIR}")
        set(index_dir
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_DOCDIR}")
    endif()

    # qtattributionsscanner
    add_custom_target(qattributionsscanner_${target}
        COMMAND ${qtattributionsscanner_bin}
        ${PROJECT_SOURCE_DIR}
        --basedir "${PROJECT_SOURCE_DIR}/.."
        --filter "QDocModule=${doc_target}"
        -o "${target_bin_dir}/codeattributions.qdoc"
        COMMENT "Scanning attributions for ${target}..."
    )

    # prepare docs target
    set(prepare_qdoc_args
        -outputdir "${qdoc_output_dir}"
        "${target_source_dir}/${qdoc_conf_path}"
        -prepare
        -indexdir "${index_dir}"
        -no-link-errors
        "${include_path_args}"
    )
    if(NOT QT_BUILD_ONLINE_DOCS)
        list(PREPEND prepare_qdoc_args
            -installdir "${QT_INSTALL_DIR}/${INSTALL_DOCDIR}"
            ${qdoc_extra_args}
        )
    endif()

    if(arg_QDOC_PREPARE_EXTRA_ARGS)
        list(APPEND prepare_qdoc_args ${arg_QDOC_PREPARE_EXTRA_ARGS})
    endif()

    if(QT_QDOC_PREPARE_EXTRA_ARGS)
        list(APPEND prepare_qdoc_args ${QT_QDOC_PREPARE_EXTRA_ARGS})
    endif()

    if(DEFINED ENV{QT_QDOC_PREPARE_EXTRA_ARGS})
        list(APPEND prepare_qdoc_args $ENV{QT_QDOC_PREPARE_EXTRA_ARGS})
    endif()

    if(DEFINED ENV{QT_INSTALL_DOCS})
        if(NOT EXISTS "$ENV{QT_INSTALL_DOCS}")
            message(FATAL_ERROR
                "Environment variable QT_INSTALL_DOCS points to a directory which does not exist:\n"
                "$ENV{QT_INSTALL_DOCS}")
        endif()
        set(qt_install_docs_env "$ENV{QT_INSTALL_DOCS}")
    elseif(QT_SUPERBUILD OR "${PROJECT_NAME}" STREQUAL "QtBase")
        set(qt_install_docs_env "${QtBase_BINARY_DIR}/${INSTALL_DOCDIR}")
    else()
        set(qt_install_docs_env
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_DOCDIR}")
    endif()

    set(qdoc_env_args
        "QT_INSTALL_DOCS=\"${qt_install_docs_env}\""
        "QT_VERSION=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
        "QT_VER=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        "QT_VERSION_TAG=${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR}${PROJECT_VERSION_PATCH}"
        "BUILDDIR=${target_bin_dir}"
    )
    if(arg_DEFINES)
        foreach(define ${arg_DEFINES})
            list(APPEND qdoc_env_args "${define}")
        endforeach()
    endif()

    add_custom_target(prepare_docs_${target}
        COMMAND ${CMAKE_COMMAND} -E env ${qdoc_env_args}
        ${qdoc_bin}
        ${prepare_qdoc_args}
        COMMENT "Running qdoc for ${target}..."
    )

    add_dependencies(prepare_docs_${target} qattributionsscanner_${target})
    if(NOT TARGET sync_all_public_headers)
        add_custom_target(sync_all_public_headers)
    endif()
    add_dependencies(prepare_docs_${target} sync_all_public_headers)

    # generate docs target
    set(generate_qdoc_args
        -outputdir "${qdoc_output_dir}"
        "${target_source_dir}/${qdoc_conf_path}"
        -generate
        -indexdir "${index_dir}"
        "${include_path_args}"
    )
    if(NOT QT_BUILD_ONLINE_DOCS)
        list(PREPEND generate_qdoc_args
            -installdir "${QT_INSTALL_DIR}/${INSTALL_DOCDIR}"
            ${qdoc_extra_args}
        )
    endif()

    if(arg_QDOC_GENERATE_EXTRA_ARGS)
        list(APPEND generate_qdoc_args ${arg_QDOC_GENERATE_EXTRA_ARGS})
    endif()

    if(QT_QDOC_GENERATE_EXTRA_ARGS)
        list(APPEND generate_qdoc_args ${QT_QDOC_GENERATE_EXTRA_ARGS})
    endif()

    if(DEFINED ENV{QT_QDOC_GENERATE_EXTRA_ARGS})
        list(APPEND generate_qdoc_args $ENV{QT_QDOC_GENERATE_EXTRA_ARGS})
    endif()

    foreach(target_prefix generate_top_level_docs generate_repo_docs generate_docs)
        set(depends_arg "")
        if(tool_dependencies_enabled)
            set(depends_arg DEPENDS ${qdoc_bin})
        endif()
        add_custom_target(${target_prefix}_${target}
            ${depends_arg}
            COMMAND ${CMAKE_COMMAND} -E env ${qdoc_env_args} ${qdoc_bin} ${generate_qdoc_args}
            COMMENT "Generating documentation for ${target}..."
        )
    endforeach()

    add_dependencies(generate_docs_${target} prepare_docs_${target})
    add_dependencies(generate_repo_docs_${target} ${qt_docs_prepare_target_name})
    add_dependencies(generate_top_level_docs_${target} prepare_docs)
    add_dependencies(generate_docs generate_top_level_docs_${target})

    # html docs target
    add_custom_target(html_docs_${target})
    add_dependencies(html_docs_${target} generate_docs_${target})

    # generate .qch
    set(qch_file_name ${doc_target}.qch)
    set(qch_file_path ${qdoc_qch_output_dir}/${qch_file_name})

    foreach(target_prefix qch_top_level_docs qch_repo_docs qch_docs)
        set(depends_arg "")
        if(tool_dependencies_enabled)
            set(depends_arg DEPENDS ${qhelpgenerator_bin})
        endif()
        add_custom_target(${target_prefix}_${target}
            ${depends_arg}
            COMMAND ${qhelpgenerator_bin}
            "${qdoc_output_dir}/${doc_target}.qhp"
            -o "${qch_file_path}"
            COMMENT "Building QtHelp files for ${target}..."
        )
    endforeach()
    add_dependencies(qch_docs_${target} generate_docs_${target})
    add_dependencies(qch_repo_docs_${target} ${qt_docs_generate_target_name})
    add_dependencies(qch_top_level_docs_${target} generate_docs)
    add_dependencies(qch_docs qch_top_level_docs_${target})

    if (QT_WILL_INSTALL)
        install(DIRECTORY "${qdoc_output_dir}/"
            DESTINATION "${INSTALL_DOCDIR}/${doc_target}"
            COMPONENT _install_html_docs_${target}
            EXCLUDE_FROM_ALL
        )

        add_custom_target(install_html_docs_${target}
            COMMAND ${CMAKE_COMMAND}
            --install "${CMAKE_BINARY_DIR}"
            --component _install_html_docs_${target}
            COMMENT "Installing html docs for target ${target}"
        )

        install(FILES "${qch_file_path}"
            DESTINATION "${INSTALL_DOCDIR}"
            COMPONENT _install_qch_docs_${target}
            EXCLUDE_FROM_ALL
        )

        add_custom_target(install_qch_docs_${target}
            COMMAND ${CMAKE_COMMAND}
            --install "${CMAKE_BINARY_DIR}"
            --component _install_qch_docs_${target}
            COMMENT "Installing qch docs for target ${target}"
        )

    else()
        # Don't need to do anything when not installing
        add_custom_target(install_html_docs_${target})
        add_custom_target(install_qch_docs_${target})
    endif()

    add_custom_target(install_docs_${target})
    add_dependencies(install_docs_${target} install_html_docs_${target} install_qch_docs_${target})

    add_custom_target(docs_${target})
    add_dependencies(docs_${target} html_docs_${target})
    add_dependencies(docs_${target} qch_docs_${target})

    add_dependencies(${qt_docs_prepare_target_name} prepare_docs_${target})
    add_dependencies(${qt_docs_generate_target_name} generate_repo_docs_${target})
    add_dependencies(${qt_docs_qch_target_name} qch_repo_docs_${target})
    add_dependencies(${qt_docs_install_html_target_name} install_html_docs_${target})
    add_dependencies(${qt_docs_install_qch_target_name} install_qch_docs_${target})

    # Make sure that the necessary tools are built when running,
    # for example 'cmake --build . --target generate_docs'.
    if(tool_dependencies_enabled)
        qt_internal_add_doc_tool_dependency(qattributionsscanner_${target} qtattributionsscanner)
        qt_internal_add_doc_tool_dependency(prepare_docs_${target} qdoc)
        qt_internal_add_doc_tool_dependency(qch_docs_${target} qhelpgenerator)
    endif()

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR add_java_documentation_args
        FORWARD_OPTIONS
            SKIP_JAVADOC
    )
    qt_internal_add_java_documentation(${target} ${add_java_documentation_args}
        OUTPUT_DIR "${qdoc_output_dir}"
    )
endfunction()

function(qt_internal_add_java_documentation target)
    if(NOT ANDROID AND NOT QT_BUILD_HOST_JAVA_DOCS)
        return()
    endif()

    set(no_value_options
        SKIP_JAVADOC
    )
    set(single_value_options
        OUTPUT_DIR
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    # Use a default output directory based on the project name.
    if(NOT DEFINED arg_OUTPUT_DIR)
        if (QT_WILL_INSTALL)
            set(arg_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${INSTALL_DOCDIR}")
        else()
            set(arg_OUTPUT_DIR "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}/${INSTALL_DOCDIR}")
        endif()
        string(APPEND arg_OUTPUT_DIR "/${PROJECT_NAME}")
    endif()

    qt_internal_collect_jar_sources(sources)

    # Bail out if we haven't found relevant sources.
    if(sources STREQUAL "")
        return()
    endif()

    if(NOT TARGET docs_android)
        add_custom_target(docs_android)
        add_custom_target(install_docs_android)
        add_dependencies(install_docs_android docs_android)

        add_custom_target(android_source_jars)
        add_custom_target(install_android_source_jars)
        add_dependencies(install_android_source_jars android_source_jars)
    endif()

    if(NOT ANDROID)
        qt_internal_set_up_build_host_java_docs()
    endif()

    if(NOT arg_SKIP_JAVADOC)
        qt_internal_add_javadoc_target(
            MODULE ${target}
            SOURCES ${sources}
            OUTPUT_DIR "${arg_OUTPUT_DIR}"
        )
    endif()

    qt_internal_create_source_jar(SOURCES ${sources} MODULE ${target})
endfunction()
