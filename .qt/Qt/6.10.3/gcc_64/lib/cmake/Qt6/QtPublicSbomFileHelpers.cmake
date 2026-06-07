# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Handles addition of binary files SPDX entries for a given target.
# Is multi-config aware.
function(_qt_internal_sbom_handle_target_binary_files target)
    set(opt_args
        NO_INSTALL
        FRAMEWORK
    )
    set(single_args
        SBOM_ENTITY_TYPE
        SPDX_ID
        LICENSE_EXPRESSION
        INSTALL_PREFIX
    )
    set(multi_args
        COPYRIGHTS
    )

    _qt_internal_sbom_get_multi_config_single_args(multi_config_single_args)
    list(APPEND single_args ${multi_config_single_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_NO_INSTALL)
        message(DEBUG "Skipping sbom target file processing ${target} because NO_INSTALL is set")
        return()
    endif()

    if(QT_SBOM_SKIP_BINARY_FILES)
        message(DEBUG "Skipping sbom target file processing ${target} because "
            "QT_SBOM_SKIP_BINARY_FILES is set")
        return()
    endif()

    set(supported_types
        QT_MODULE
        QT_PLUGIN
        QT_APP
        QT_TOOL
        QT_THIRD_PARTY_MODULE
        QT_THIRD_PARTY_SOURCES
        SYSTEM_LIBRARY
        QT_TRANSLATIONS
        QT_RESOURCES
        QT_CUSTOM
        QT_CUSTOM_NO_INFIX

        # This will be meant for user projects, and are not currently used by Qt's sbom.
        THIRD_PARTY_LIBRARY
        THIRD_PARTY_LIBRARY_WITH_FILES
        THIRD_PARTY_SOURCES
        SBOM_PROJECT
        EXECUTABLE
        LIBRARY
        TRANSLATIONS
        RESOURCES
        BUILD_TOOL
        CUSTOM
        CUSTOM_NO_INFIX
    )

    if(NOT arg_SBOM_ENTITY_TYPE IN_LIST supported_types)
        message(FATAL_ERROR
            "Unsupported target TYPE '${arg_SBOM_ENTITY_TYPE}' for target '${target}' during "
            "SBOM creation.")
    endif()

    set(types_without_binary_files
        QT_THIRD_PARTY_SOURCES
        QT_TRANSLATIONS
        QT_RESOURCES
        QT_CUSTOM
        QT_CUSTOM_NO_INFIX
        SYSTEM_LIBRARY
        THIRD_PARTY_LIBRARY
        THIRD_PARTY_SOURCES
        SBOM_PROJECT
        TRANSLATIONS
        RESOURCES
        BUILD_TOOL
        CUSTOM
        CUSTOM_NO_INFIX
    )

    get_target_property(target_type ${target} TYPE)

    if(arg_SBOM_ENTITY_TYPE IN_LIST types_without_binary_files)
        message(DEBUG "Target ${target} has no binary files to reference in the SBOM "
            "because it has the ${arg_SBOM_ENTITY_TYPE} type.")
        return()
    endif()

    if(target_type STREQUAL "INTERFACE_LIBRARY")
        message(DEBUG "Target ${target} has no binary files to reference in the SBOM "
            "because it is an INTERFACE_LIBRARY.")
        return()
    endif()

    get_target_property(excluded_via_property ${target} _qt_internal_excluded_from_default_target)
    if(excluded_via_property OR QT_INTERNAL_TEST_TARGETS_EXCLUDE_FROM_ALL)
        message(DEBUG "Target ${target} has no binary files to reference in the SBOM "
            "because it was excluded from the default 'all' target.")
        return()
    endif()

    if(NOT arg_SPDX_ID)
        message(FATAL_ERROR "SPDX_ID must be set")
    endif()
    set(package_spdx_id "${arg_SPDX_ID}")

    set(file_common_options "")

    list(APPEND file_common_options PACKAGE_SPDX_ID "${package_spdx_id}")
    list(APPEND file_common_options PACKAGE_TYPE "${arg_SBOM_ENTITY_TYPE}")

    if(arg_COPYRIGHTS)
        list(APPEND file_common_options COPYRIGHTS "${arg_COPYRIGHTS}")
    endif()

    if(arg_LICENSE_EXPRESSION)
        list(APPEND file_common_options LICENSE_EXPRESSION "${arg_LICENSE_EXPRESSION}")
    endif()

    if(arg_INSTALL_PREFIX)
        list(APPEND file_common_options INSTALL_PREFIX "${arg_INSTALL_PREFIX}")
    endif()

    set(path_suffix "$<TARGET_FILE_NAME:${target}>")

    if(arg_FRAMEWORK)
        set(library_path_kind FRAMEWORK_PATH)
    else()
        set(library_path_kind LIBRARY_PATH)
    endif()

    if(arg_SBOM_ENTITY_TYPE STREQUAL "QT_TOOL"
            OR arg_SBOM_ENTITY_TYPE STREQUAL "QT_APP"
            OR arg_SBOM_ENTITY_TYPE STREQUAL "EXECUTABLE")

        set(valid_executable_types
            "EXECUTABLE"
        )
        if(ANDROID)
            list(APPEND valid_executable_types "MODULE_LIBRARY")
        endif()
        if(NOT target_type IN_LIST valid_executable_types)
            message(FATAL_ERROR "Unsupported target type of target '${target}': ${target_type}")
        endif()

        get_target_property(app_is_bundle ${target} MACOSX_BUNDLE)
        if(app_is_bundle)
            _qt_internal_get_executable_bundle_info(bundle "${target}")
            _qt_internal_path_join(path_suffix "${bundle_contents_binary_dir}" "${path_suffix}")
        endif()

        _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
            PATH_KIND RUNTIME_PATH
            PATH_SUFFIX "${path_suffix}"
            OPTIONS ${file_common_options}
        )
    elseif(arg_SBOM_ENTITY_TYPE STREQUAL "QT_PLUGIN")
        if(NOT (target_type STREQUAL "SHARED_LIBRARY"
                OR target_type STREQUAL "STATIC_LIBRARY"
                OR target_type STREQUAL "MODULE_LIBRARY"))
            message(FATAL_ERROR "Unsupported target type: ${target_type}")
        endif()

        _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
            PATH_KIND INSTALL_PATH
            PATH_SUFFIX "${path_suffix}"
            OPTIONS ${file_common_options}
        )
    elseif(arg_SBOM_ENTITY_TYPE STREQUAL "QT_MODULE"
            OR arg_SBOM_ENTITY_TYPE STREQUAL "QT_THIRD_PARTY_MODULE"
            OR arg_SBOM_ENTITY_TYPE STREQUAL "LIBRARY"
            OR arg_SBOM_ENTITY_TYPE STREQUAL "THIRD_PARTY_LIBRARY_WITH_FILES"
        )
        if(WIN32 AND target_type STREQUAL "SHARED_LIBRARY")
            _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
                PATH_KIND RUNTIME_PATH
                PATH_SUFFIX "${path_suffix}"
                OPTIONS ${file_common_options}
            )

            _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
                PATH_KIND ARCHIVE_PATH
                PATH_SUFFIX "$<TARGET_LINKER_FILE_NAME:${target}>"
                OPTIONS
                    ${file_common_options}
                    IMPORT_LIBRARY
                    # OPTIONAL because on Windows the import library might not always be present,
                    # because no symbols are exported.
                    OPTIONAL
            )
        elseif(target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "STATIC_LIBRARY")
            _qt_internal_sbom_handle_multi_config_target_binary_file(${target}
                PATH_KIND "${library_path_kind}"
                PATH_SUFFIX "${path_suffix}"
                OPTIONS ${file_common_options}
            )
        else()
            message(FATAL_ERROR "Unsupported target type: ${target_type}")
        endif()
    endif()
endfunction()

# Add a binary file of a target to the sbom (e.g a shared library or an executable).
# Adds relationships to the SBOM that the binary file was generated from its source files,
# as well as relationship to the owning package.
# TODO: Consider merging the common parts with _qt_internal_sbom_add_custom_file somehow.
function(_qt_internal_sbom_add_binary_file target file_path)
    set(opt_args
        OPTIONAL
        IMPORT_LIBRARY
    )
    set(single_args
        PACKAGE_SPDX_ID
        PACKAGE_TYPE
        LICENSE_EXPRESSION
        CONFIG
        INSTALL_PREFIX
    )
    set(multi_args
        COPYRIGHTS
    )
    cmake_parse_arguments(PARSE_ARGV 2 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PACKAGE_SPDX_ID)
        message(FATAL_ERROR "PACKAGE_SPDX_ID must be set")
    endif()

    set(file_common_options "")

    if(arg_COPYRIGHTS)
        list(JOIN arg_COPYRIGHTS "\n" copyrights)
        list(APPEND file_common_options COPYRIGHT "<text>${copyrights}</text>")
    endif()

    if(arg_LICENSE_EXPRESSION)
        list(APPEND file_common_options LICENSE "${arg_LICENSE_EXPRESSION}")
    endif()

    if(arg_INSTALL_PREFIX)
        list(APPEND file_common_options INSTALL_PREFIX "${arg_INSTALL_PREFIX}")
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(spdx_id_suffix "${arg_CONFIG}")
        set(config_to_install_option CONFIG ${arg_CONFIG})
    else()
        set(spdx_id_suffix "")
        set(config_to_install_option "")
    endif()

    set(file_infix "")
    if(arg_IMPORT_LIBRARY)
        set(file_infix "-ImportLibrary")
    endif()

    # We kind of have to add the package infix into the file spdx, otherwise we get file system
    # collisions for cases like the qml tool and Qml library, and apparently cmake's file(GENERATE)
    # is case insensitive for file names.
    _qt_internal_sbom_get_package_infix("${arg_PACKAGE_TYPE}" package_infix)

    _qt_internal_sbom_get_file_spdx_id(
        "${package_infix}-${target}-${file_infix}-${spdx_id_suffix}" spdx_id)

    set(optional "")
    if(arg_OPTIONAL)
        set(optional OPTIONAL)
    endif()

    # Add relationship from owning package.
    set(relationships "${arg_PACKAGE_SPDX_ID} CONTAINS ${spdx_id}")

    # Add source file relationships from which the binary file was generated.
    if(NOT QT_SBOM_SKIP_SOURCE_FILES)
        _qt_internal_sbom_add_target_source_files("${target}" "${spdx_id}" source_relationships)
        if(source_relationships)
            list(APPEND relationships "${source_relationships}")
        endif()
    else()
        message(DEBUG "Skipping sbom source file processing for file '${file_path}' because "
            "QT_SBOM_SKIP_SOURCE_FILES is set")
    endif()

    set(glue "\nRelationship: ")
    # Replace semicolon with $<SEMICOLON> to avoid errors when passing into sbom_add.
    string(REPLACE ";" "$<SEMICOLON>" relationships "${relationships}")

    # Glue the relationships at generation time, because there some source file relationships
    # will be conditional on genexes, and evaluate to an empty value, and we want to discard
    # such relationships.
    set(relationships "$<JOIN:${relationships},${glue}>")
    set(relationship_option RELATIONSHIP "${relationships}")

    # Add the actual binary file to the latest package.
    _qt_internal_sbom_generate_add_file(
        FILENAME "${file_path}"
        FILETYPE BINARY ${optional}
        SPDXID "${spdx_id}"
        PARENT_PACKAGE_SPDXID "${arg_PACKAGE_SPDX_ID}"
        ${file_common_options}
        ${config_to_install_option}
        ${relationship_option}
    )
endfunction()

# Add a list of to-be-installed files that should appear in the files section of the target's
# SBOM document.
# Supports multiple calls with the same target name.
# Each call is handled as a separate set of files.
# For options that can be passed, see the doc-comment of
# _qt_internal_sbom_handle_target_custom_file_set.
function(_qt_internal_sbom_add_files target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "The target ${target} does not exist.")
    endif()

    get_target_property(file_sets_count "${target}" _qt_sbom_custom_file_sets_count)
    if(NOT file_sets_count)
        set(file_sets_count 0)
    endif()

    set_property(TARGET "${target}"
        APPEND PROPERTY _qt_sbom_custom_file_set_${file_sets_count} "${ARGN}")

    math(EXPR file_sets_count "${file_sets_count}+1")
    set_property(TARGET "${target}" PROPERTY _qt_sbom_custom_file_sets_count "${file_sets_count}")
endfunction()

# Handles addition of custom file SPDX entries for a given target, by processing all the collected
# file sets so far.
# Applies the passed license and copyright info to all collected files.
#
# Options that can be passed.
#
# NO_INSTALL - if set, custom file processing is skipped, because the files will not be installed.
#
# PACKAGE_TYPE - the type of the package that the files belong to, is used to compute an infix
# for the file spdx id.
#
# PACKAGE_SPDX_ID - the package spdx id is used to add a relationship between the package and file.
#
# LICENSE_EXPRESSION - a license expression to apply to the files.
#
# INSTALL_PREFIX - the install prefix for the files, this is usually the install prefix of qt.
#
# COPYRIGHTS - a list of copyright strings to apply to the files.
function(_qt_internal_sbom_handle_target_custom_files target)
    set(opt_args
        NO_INSTALL
    )
    set(single_args
        PACKAGE_TYPE
        PACKAGE_SPDX_ID
        LICENSE_EXPRESSION
        INSTALL_PREFIX
    )
    set(multi_args
        COPYRIGHTS
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Nothing to process if no file sets were defined.
    get_target_property(file_sets_count "${target}" _qt_sbom_custom_file_sets_count)
    if(NOT file_sets_count)
        return()
    endif()

    if(arg_NO_INSTALL)
        message(DEBUG "Skipping sbom custom file processing for ${target} because NO_INSTALL is "
            "set")
        return()
    endif()

    if(QT_SBOM_SKIP_CUSTOM_FILES)
        message(DEBUG "Skipping sbom custom file processing ${target} because "
            "QT_SBOM_SKIP_BINARY_FILES is set")
        return()
    endif()

    if(NOT arg_PACKAGE_SPDX_ID)
        message(FATAL_ERROR "PACKAGE_SPDX_ID must be set")
    endif()

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR file_common_options
        FORWARD_SINGLE
            PACKAGE_TYPE
            PACKAGE_SPDX_ID
            LICENSE_EXPRESSION
            INSTALL_PREFIX
        FORWARD_MULTI
            COPYRIGHTS
    )

    # Subtract -1, because foreach(RANGE is inclusive).
    math(EXPR file_sets_count "${file_sets_count}-1")
    foreach(file_set_index RANGE ${file_sets_count})
        get_target_property(file_set_args "${target}" _qt_sbom_custom_file_set_${file_set_index})

        if(NOT file_set_args)
            message(FATAL_ERROR "No arguments were specified for SBOM custom file set for "
                "${target}")
        endif()

        _qt_internal_sbom_handle_target_custom_file_set("${target}"
            ${file_common_options} ${file_set_args})
    endforeach()
endfunction()

# Processes a file set of custom files for which to include SBOM info.
# The
#   NO_INSTALL
#   PACKAGE_TYPE
#   PACKAGE_SPDX_ID
#   LICENSE_EXPRESSION
#   INSTALL_PREFIX
#   COPYRIGHTS
# options are the same as in _qt_internal_sbom_handle_target_custom_files, and are actually
# forwarded from that function, because they might be set at the target level.
#
# In addition, more options can be passed when called via qt_internal_sbom_add_files:
#
# They are:
#
# FILE_TYPE - the type of each provided file. Supported types can be found in the implementation of
# _qt_internal_sbom_get_spdx_v2_3_file_type_for_file().
# Some examples are QT_TRANSLATION, QT_TRANSLATIONS_CATALOG, QT_RESOURCE.
#
# FILES - a list of file paths to include in the SBOM.
#
# DIRECTORIES - a list of directories which will be file(GLOB_RECURSE)d to find files to include in
# the SBOM.
#
# SOURCE_FILES - which source files were used to generate the custom files. All source files apply
# to each input file.
#
# SOURCE_FILES_PER_INPUT_FILE - for each index i in FILES, the corresponding source files are
# in SOURCE_FILES[i], so that each input gets exactly one source file.
# This is provided as an option, to prevent performance overhead from having to add a
# custom file set for each new source file, when dealing with translations that have a
# 1-to-1 ts->qm relationship.
#
# There is also a set of multi config aware options that can be set, like
#   INSTALL_PATH
#   INSTALL_PATH_<CONFIG>
# which should be the relative install dir path where the
# files will be installed, relative to $ENV{DESTDIR}/${CMAKE_INSTALL_PREFIX}.
function(_qt_internal_sbom_handle_target_custom_file_set target)
    set(opt_args
        ""
    )
    set(single_args
        FILE_TYPE
        PACKAGE_SPDX_ID
        PACKAGE_TYPE
        LICENSE_EXPRESSION
        INSTALL_PREFIX
    )
    set(multi_args
        COPYRIGHTS
        FILES
        DIRECTORIES
        SOURCE_FILES
        SOURCE_FILES_PER_INPUT_FILE
    )

    # Don't explicitly forward the multi config single args, but still parse them.
    # They will be accessed from the current scope directly.
    set(single_args_without_multi_config_args "${single_args}")
    _qt_internal_sbom_get_multi_config_single_args(multi_config_single_args)
    list(APPEND single_args ${multi_config_single_args})

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    # No custom files to process.
    if(NOT arg_FILES AND NOT arg_DIRECTORIES)
        return()
    endif()

    # Don't forward the FILES and DIRECTORIES options.
    set(multi_args_without_files "${multi_args}")
    list(REMOVE_ITEM multi_args_without_files FILES DIRECTORIES)

    # Handle the case where we have one source file per input file.
    if(arg_SOURCE_FILES_PER_INPUT_FILE)
        if(NOT arg_FILES)
            message(FATAL_ERROR "SOURCE_FILES_PER_INPUT_FILE can only be set if FILES is set.")
        endif()
        list(LENGTH arg_FILES files_count)
        list(LENGTH arg_SOURCE_FILES_PER_INPUT_FILE source_files_count)
        if(NOT files_count EQUAL source_files_count)
            message(FATAL_ERROR "The number of files passed to SOURCE_FILES must match the number"
                "of files passed to SOURCE_FILES_PER_INPUT_FILE.")
        endif()

        # Don't forward all of the source files, but rather one per input file.
        list(REMOVE_ITEM multi_args_without_files SOURCE_FILES_PER_INPUT_FILE)
    endif()

    _qt_internal_forward_function_args(
        FORWARD_PREFIX arg
        FORWARD_OUT_VAR forward_args
        FORWARD_SINGLE
            ${single_args_without_multi_config_args}
        FORWARD_MULTI
            ${multi_args_without_files}
    )

    set(files "")
    if(arg_FILES)
        list(APPEND files ${arg_FILES})
    endif()

    set(directories "")
    if(arg_DIRECTORIES)
        list(APPEND directories ${arg_DIRECTORIES})
    endif()

    set(relative_file_paths "")

    # For each file in FILES, we only add the file name as a suffix, and not the path relative to
    # the current source dir, because that's how install(FILES) behaves. The final installed
    # destination is ${arg_INSTALL_PATH}/${file_name}.
    foreach(file_path IN LISTS files)
        get_filename_component(relative_file_path "${file_path}" NAME)
        list(APPEND relative_file_paths "${relative_file_path}")
    endforeach()

    # For each file globbed in DIRECTORIES, we add the file path relative to the current source dir
    # + all components of the given DIRECTORY except for the last one.
    # That's how install(DIRECTORY) behaves.
    # So the final installed destination is
    # ${arg_INSTALL_PATH}/${directory_without_last_component}/${path_to_file}.
    foreach(directory IN LISTS directories)
        file(GLOB_RECURSE files_in_directory "${directory}/*")
        set(directory_abs_path "${CMAKE_CURRENT_SOURCE_DIR}/${directory}")
        get_filename_component(parent_dir "${directory_abs_path}" DIRECTORY)
        foreach(file_path IN LISTS files_in_directory)
            file(RELATIVE_PATH relative_file_path "${parent_dir}" "${file_path}")
            list(APPEND relative_file_paths "${relative_file_path}")
        endforeach()
    endforeach()

    set(file_index 0)
    foreach(relative_file_path IN LISTS relative_file_paths)
        set(per_file_forward_args "")

        if(arg_SOURCE_FILES_PER_INPUT_FILE)
            list(GET arg_SOURCE_FILES_PER_INPUT_FILE "${file_index}" source_file)
            list(APPEND per_file_forward_args SOURCE_FILES "${source_file}")
        endif()

        # The multi_config_single_args are deliberately not forwarded, but are available in this
        # function scope, for direct access in the called function scope, because
        # cmake_parse_arguments can't handle:
        #   PATH_KIND "INSTALL_PATH"
        #   INSTALL_PATH "/some_path"
        # the parsing gets confused by what's the option and what's the value.
        _qt_internal_sbom_handle_multi_config_custom_file(${target}
            PATH_KIND "INSTALL_PATH"
            PATH_SUFFIX "${relative_file_path}"
            OPTIONS
                ${forward_args}
                ${per_file_forward_args}
        )
        math(EXPR file_index "${file_index}+1")
    endforeach()
endfunction()

# Helper function to add a custom file to the sbom, while handling multi-config and different
# kind of paths.
# In multi-config builds, we assume that the non-default config file will be optional, because it
# might not be installed.
#
# Expects the parent scope to contain the ${PATH_KIND} and ${PATH_KIND}_<CONFIG> variables.
# Examples are INSTALL_PATH or INSTALL_PATH_DEBUG.
# They can't be forwarded as options because of cmake_parse_arguments parsing issues when a value
# can be the same as a key. See comment in implementation of
#_qt_internal_sbom_handle_target_custom_file_set.
function(_qt_internal_sbom_handle_multi_config_custom_file target)
    set(opt_args "")
    set(single_args
        PATH_KIND
        PATH_SUFFIX
    )
    set(multi_args
        OPTIONS
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    elseif(CMAKE_BUILD_TYPE)
        set(configs "${CMAKE_BUILD_TYPE}")
    else()
        set(configs "<EMPTY_CONFIG>")
    endif()

    foreach(config IN LISTS configs)
        _qt_internal_sbom_get_and_check_multi_config_aware_single_arg_option(
            arg "${arg_PATH_KIND}" "${config}" resolved_path)
        _qt_internal_sbom_get_target_file_is_optional_in_multi_config("${config}" is_optional)
        _qt_internal_path_join(file_path "${resolved_path}" "${arg_PATH_SUFFIX}")
        _qt_internal_sbom_add_custom_file(
            "${target}"
            "${file_path}"
            ${arg_OPTIONS}
            ${is_optional}
            CONFIG ${config}
        )
    endforeach()
endfunction()

# Adds one custom file with the given relative install path into the SBOM document.
# Will embed GENERATED_FROM source file relationships if a list of source files is specified.
function(_qt_internal_sbom_add_custom_file target installed_file_relative_path)
    set(opt_args
        OPTIONAL
    )
    set(single_args
        PACKAGE_SPDX_ID
        PACKAGE_TYPE
        LICENSE_EXPRESSION
        INSTALL_PREFIX
        FILE_TYPE
        CONFIG
    )
    set(multi_args
        COPYRIGHTS
        SOURCE_FILES
    )

    cmake_parse_arguments(PARSE_ARGV 2 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PACKAGE_SPDX_ID)
        message(FATAL_ERROR "PACKAGE_SPDX_ID must be set")
    endif()

    set(file_common_options "")

    if(arg_COPYRIGHTS)
        list(JOIN arg_COPYRIGHTS "\n" copyrights)
        list(APPEND file_common_options COPYRIGHT "<text>${copyrights}</text>")
    endif()

    if(arg_LICENSE_EXPRESSION)
        list(APPEND file_common_options LICENSE "${arg_LICENSE_EXPRESSION}")
    endif()

    if(arg_INSTALL_PREFIX)
        list(APPEND file_common_options INSTALL_PREFIX "${arg_INSTALL_PREFIX}")
    endif()

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(spdx_id_suffix "${arg_CONFIG}")
        set(config_to_install_option CONFIG ${arg_CONFIG})
    else()
        set(spdx_id_suffix "")
        set(config_to_install_option "")
    endif()

    if(arg_FILE_TYPE)
        _qt_internal_sbom_get_spdx_v2_3_file_type_for_file(file_type "${arg_FILE_TYPE}")
    else()
        set(file_type "OTHER")
    endif()

    # Append a short hash based on the installed relative path of the file, to avoid spdx id
    # clashes for files with the same name installed into different dirs.
    get_filename_component(file_name "${installed_file_relative_path}" NAME)
    string(SHA1 rel_path_hash "${installed_file_relative_path}")
    string(SUBSTRING "${rel_path_hash}" 0 4 short_hash)
    set(file_name "${file_name}-${short_hash}")

    _qt_internal_sbom_get_package_infix("${arg_PACKAGE_TYPE}" package_infix)

    _qt_internal_sbom_get_file_spdx_id(
        "${package_infix}-${target}-${file_name}-${spdx_id_suffix}" spdx_id)

    # Add relationship from owning package.
    set(relationships "${arg_PACKAGE_SPDX_ID} CONTAINS ${spdx_id}")

    # Add source file relationships from which the custom file was generated.
    set(sources_option "")

    if(arg_SOURCE_FILES)
        set(sources_option SOURCES ${arg_SOURCE_FILES})
    endif()

    # Add source file relationships from which the binary file was generated.
    if(NOT QT_SBOM_SKIP_SOURCE_FILES)
        _qt_internal_sbom_add_source_files(
            ${sources_option}
            SPDX_ID "${spdx_id}"
            OUT_RELATIONSHIPS_VAR source_relationships
        )
    else()
        message(DEBUG "Skipping sbom source file processing for file "
            "'${installed_file_relative_path}' because QT_SBOM_SKIP_SOURCE_FILES is set")
    endif()

    if(source_relationships)
        list(APPEND relationships "${source_relationships}")
    endif()

    set(glue "\nRelationship: ")
    # Replace semicolon with $<SEMICOLON> to avoid errors when passing into sbom_add.
    string(REPLACE ";" "$<SEMICOLON>" relationships "${relationships}")

    # Glue the relationships at generation time, because there some source file relationships
    # will be conditional on genexes, and evaluate to an empty value, and we want to discard
    # such relationships.
    set(relationships "$<JOIN:${relationships},${glue}>")
    set(relationship_option RELATIONSHIP "${relationships}")

    _qt_internal_sbom_generate_add_file(
        FILENAME "${installed_file_relative_path}"
        FILETYPE "${file_type}" ${optional}
        SPDXID "${spdx_id}"
        PARENT_PACKAGE_SPDXID "${arg_PACKAGE_SPDX_ID}"
        ${file_common_options}
        ${config_to_install_option}
        ${relationship_option}
    )
endfunction()

# Maps an arbitrary file type to a spdx v2.3 file type.
# There is a list of known SPDX types, and custom Qt ones.
# Any other type is mapped to OTHER.
# The mapping might change when we start generating spdx v3.0 documents.
function(_qt_internal_sbom_get_spdx_v2_3_file_type_for_file out_var file_type_in)
    set(spdx_v2_3_file_types
        SOURCE
        BINARY
        ARCHIVE
        APPLICATION
        AUDIO
        IMAGE
        TEXT
        VIDEO
        DOCUMENTATION
        SPDX
        OTHER
    )

    # No semantic meaning at the moment, but we might want to map the values to something else
    # when we port to SPDX v3.0+.
    set(qt_file_types
        QT_TRANSLATION
        QT_TRANSLATIONS_CATALOG
        QT_RESOURCE
        TRANSLATION
        RESOURCE
        CUSTOM
    )

    if(file_type_in IN_LIST spdx_v2_3_file_types)
        set(file_type "${file_type_in}")
    elseif(file_type_in IN_LIST qt_file_types)
        set(file_type OTHER)
    else()
        set(file_type OTHER)
    endif()

    set(${out_var} "${file_type}" PARENT_SCOPE)
endfunction()

# Takes a relative or absolute path and maps it to a reproducible path that is relative to
# the project source or build dir.
function(_qt_internal_sbom_map_path_to_reproducible_relative_path out_var)
    set(opt_args "")
    set(single_args
        PATH
        REPO_PROJECT_NAME_LOWERCASE
        OUT_SUCCESS
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(is_in_source_dir FALSE)
    set(is_in_build_dir FALSE)
    set(is_in_sysroot_dir FALSE)

    if(NOT arg_REPO_PROJECT_NAME_LOWERCASE)
        _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name)
    else()
        set(repo_project_name "${arg_REPO_PROJECT_NAME_LOWERCASE}")
    endif()

    if(NOT DEFINED arg_PATH)
        message(FATAL_ERROR "PATH must be set")
    endif()
    set(path "${arg_PATH}")

    set(handled FALSE)

    if(path MATCHES "$<.+>")
        # TODO: Paths wrapped in genexes are usually absolute paths that we also want to handle,
        # but can't at configure time. We'll need a separate processing step at build time.
        # Keep these as is for now, but signal failure of handling.
        set(path_out "${path}")
    else()
        if(IS_ABSOLUTE "${path}")
            set(path_in "${path}")

            _qt_internal_path_is_prefix(PROJECT_SOURCE_DIR "${path}" is_in_source_dir)
            if(NOT is_in_source_dir)
                _qt_internal_path_is_prefix(PROJECT_BINARY_DIR "${path}" is_in_build_dir)
            endif()
            if(NOT is_in_source_dir AND NOT is_in_build_dir)
                _qt_internal_path_is_prefix(CMAKE_INSTALL_PREFIX "${path}" is_in_prefix_dir)
            endif()
            if(CMAKE_SYSROOT
                AND NOT is_in_source_dir
                AND NOT is_in_build_dir
                AND NOT is_in_prefix_dir)
                _qt_internal_path_is_prefix(CMAKE_SYSROOT "${path}" is_in_sysroot_dir)
            endif()
        else()
            # We consider relative paths to be relative to the current source dir.
            set(is_in_source_dir TRUE)
            set(path_in "${CMAKE_CURRENT_SOURCE_DIR}/${path}")
        endif()

        # Resolve any .. and replace the absolute path with a path relative to the source dir
        # or build dir, prefixed with a root marker.
        get_filename_component(path_in_real "${path_in}" REALPATH)

        # Replace the absolute prefixes with markers.
        if(is_in_source_dir)
            set(handled TRUE)
            set(marker "/src_dir")
            string(REPLACE "${PROJECT_SOURCE_DIR}/" "${marker}/${repo_project_name}/"
                path_out "${path_in_real}")
        elseif(is_in_build_dir)
            set(handled TRUE)
            set(marker "/build_dir")
            string(REPLACE "${PROJECT_BINARY_DIR}/" "${marker}/${repo_project_name}/"
                path_out "${path_in_real}")
        elseif(is_in_prefix_dir)
            # In a non-prefix build, we put synced headers into the qtbase build dir, aka the prefix
            # dir. We should match and replace such paths as well.
            set(handled TRUE)
            set(marker "/install_dir")
            string(REPLACE "${CMAKE_INSTALL_PREFIX}/" "${marker}/" path_out "${path_in_real}")
        elseif(is_in_sysroot_dir)
            set(handled TRUE)
            set(marker "/sysroot_dir")
            string(REPLACE "${CMAKE_SYSROOT}/" "${marker}/" path_out "${path_in_real}")
        else()
            # If it's not a source dir, a build dir, or install dir, it might be some kind of
            # weird genex or marker that we don't handle yet.
            message(DEBUG "Couldn't map the path '${path_in_real}' to a reproducible relative "
                "path, because it is not in the source, build or install dir.")
            set(path_out "${path_in_real}")
        endif()
    endif()

    set(${out_var} "${path_out}" PARENT_SCOPE)
    if(arg_OUT_SUCCESS)
        set(${arg_OUT_SUCCESS} "${handled}" PARENT_SCOPE)
    endif()
endfunction()

# Collect source file "generated from" relationship comments for a given target file.
function(_qt_internal_sbom_add_target_source_files target spdx_id out_relationships)
    get_target_property(sources ${target} SOURCES)
    if(NOT sources)
        set(sources "")
    endif()
    list(REMOVE_DUPLICATES sources)

    set(sources_option "")
    if(sources)
        set(sources_option SOURCES ${sources})
    endif()

    _qt_internal_sbom_add_source_files(
        ${sources_option}
        SPDX_ID "${spdx_id}"
        OUT_RELATIONSHIPS_VAR relationships
    )

    set(${out_relationships} "${relationships}" PARENT_SCOPE)
endfunction()

# Collect source file "generated from" relationship comments for the given sources.
function(_qt_internal_sbom_add_source_files)
    set(opt_args "")
    set(single_args
        SPDX_ID
        OUT_RELATIONSHIPS_VAR
    )
    set(multi_args
        SOURCES
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_SPDX_ID)
        message(FATAL_ERROR "SPDX_ID must be set")
    endif()

    if(NOT arg_OUT_RELATIONSHIPS_VAR)
        message(FATAL_ERROR "OUT_RELATIONSHIPS_VAR must be set")
    endif()

    if(NOT arg_SOURCES)
        set(sources "")
    else()
        set(sources "${arg_SOURCES}")
    endif()

    set(relationships "")

    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)

    foreach(source IN LISTS sources)
        # Filter out $<TARGET_OBJECTS: genexes>.
        if(source MATCHES "^\\$<TARGET_OBJECTS:.*>$")
            continue()
        endif()

        # Filter out prl files.
        if(source MATCHES "\.prl$")
            continue()
        endif()

        # Filter out pkg-config. pc files.
        if(source MATCHES "\.pc$")
            continue()
        endif()

        # Filter out metatypes .json.gen files.
        if(source MATCHES "\.json\.gen$")
            continue()
        endif()

        _qt_internal_sbom_map_path_to_reproducible_relative_path(source_path
            PATH "${source}"
            REPO_PROJECT_NAME_LOWERCASE "${repo_project_name_lowercase}"
        )

        set(source_entry
"${spdx_id} GENERATED_FROM NOASSERTION\nRelationshipComment: ${source_path}"
        )
        set(source_non_empty "$<BOOL:${source_path}>")
        # Some sources are conditional on genexes, so we evaluate them.
        set(relationship "$<${source_non_empty}:$<GENEX_EVAL:${source_entry}>>")
        list(APPEND relationships "${relationship}")
    endforeach()

    set(${arg_OUT_RELATIONSHIPS_VAR} "${relationships}" PARENT_SCOPE)
endfunction()

# Collects app bundle related information and paths from an executable's target properties.
# Output variables:
#    <out_var>_name bundle base name, e.g. 'Linguist'.
#    <out_var>_dir_name bundle dir name, e.g. 'Linguist.app'.
#    <out_var>_contents_dir bundle contents dir, e.g. 'Linguist.app/Contents'
#    <out_var>_contents_binary_dir bundle contents dir, e.g. 'Linguist.app/Contents/MacOS'
function(_qt_internal_get_executable_bundle_info out_var target)
    get_target_property(target_type ${target} TYPE)
    if(NOT "${target_type}" STREQUAL "EXECUTABLE")
        message(FATAL_ERROR "The target ${target} is not an executable")
    endif()

    get_target_property(output_name ${target} OUTPUT_NAME)
    if(NOT output_name)
        set(output_name "${target}")
    endif()

    set(${out_var}_name "${output_name}")
    set(${out_var}_dir_name "${${out_var}_name}.app")
    set(${out_var}_contents_dir "${${out_var}_dir_name}/Contents")
    set(${out_var}_contents_binary_dir "${${out_var}_contents_dir}/MacOS")

    set(${out_var}_name "${${out_var}_name}" PARENT_SCOPE)
    set(${out_var}_dir_name "${${out_var}_dir_name}" PARENT_SCOPE)
    set(${out_var}_contents_dir "${${out_var}_contents_dir}" PARENT_SCOPE)
    set(${out_var}_contents_binary_dir "${${out_var}_contents_binary_dir}" PARENT_SCOPE)
endfunction()

# Helper function to add binary file to the sbom, while handling multi-config and different
# kind of paths.
# In multi-config builds, we assume that the non-default config file will be optional, because it
# might not be installed (the case for debug tools and apps in debug-and-release builds).
function(_qt_internal_sbom_handle_multi_config_target_binary_file target)
    set(opt_args "")
    set(single_args
        PATH_KIND
        PATH_SUFFIX
    )
    set(multi_args
        OPTIONS
    )

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(configs ${CMAKE_CONFIGURATION_TYPES})
    elseif(CMAKE_BUILD_TYPE)
        set(configs "${CMAKE_BUILD_TYPE}")
    else()
        set(configs "<EMPTY_CONFIG>")
    endif()

    foreach(config IN LISTS configs)
        _qt_internal_sbom_get_and_check_multi_config_aware_single_arg_option(
            arg "${arg_PATH_KIND}" "${config}" resolved_path)
        _qt_internal_sbom_get_target_file_is_optional_in_multi_config("${config}" is_optional)
        _qt_internal_path_join(file_path "${resolved_path}" "${arg_PATH_SUFFIX}")
        _qt_internal_sbom_add_binary_file(
            "${target}"
            "${file_path}"
            ${arg_OPTIONS}
            ${is_optional}
            CONFIG ${config}
        )
    endforeach()
endfunction()

# Helper to retrieve a list of multi-config aware option names that can be parsed by the
# file handling functions.
# For example in single config we need to parse RUNTIME_PATH, in multi-config we need to parse
# RUNTIME_PATH_DEBUG and RUNTIME_PATH_RELEASE.
#
# Result is cached in a global property.
function(_qt_internal_sbom_get_multi_config_single_args out_var)
    get_cmake_property(single_args
        _qt_internal_sbom_multi_config_single_args)

    if(single_args)
        set(${out_var} ${single_args} PARENT_SCOPE)
        return()
    endif()

    set(single_args "")

    set(single_args_to_process
        INSTALL_PATH
        RUNTIME_PATH
        LIBRARY_PATH
        ARCHIVE_PATH
        FRAMEWORK_PATH
    )

    list(APPEND single_args "${single_args_to_process}")

    # We need to process multi config args even in a single config build, because there might be API
    # calls that specify them directly. Use a default set of multi config configs.
    set(configs Release RelWithDebInfo MinSizeRel Debug)

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config AND CMAKE_CONFIGURATION_TYPES)
        list(APPEND configs ${CMAKE_CONFIGURATION_TYPES})
    endif()

    list(REMOVE_DUPLICATES configs)

    foreach(config IN LISTS configs)
        string(TOUPPER ${config} config_upper)
        foreach(single_arg IN LISTS single_args_to_process)
            list(APPEND single_args "${single_arg}_${config_upper}")
        endforeach()
    endforeach()

    set_property(GLOBAL PROPERTY
        _qt_internal_sbom_multi_config_single_args "${single_args}")
    set(${out_var} ${single_args} PARENT_SCOPE)
endfunction()

# Helper to apped a an option and a value to a list of options, while being multi-config aware.
# It appends e.g. either RUNTIME_PATH foo or RUNTIME_PATH_DEBUG foo to the out_var_args variable.
function(_qt_internal_sbom_append_multi_config_aware_single_arg_option
        arg_name arg_value config out_var_args)
    set(values "${${out_var_args}}")

    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        string(TOUPPER ${config} config_upper)
        list(APPEND values "${arg_name}_${config_upper}" "${arg_value}")
    else()
        list(APPEND values "${arg_name}" "${arg_value}")
    endif()

    set(${out_var_args} "${values}" PARENT_SCOPE)
endfunction()

# Helper to check whether a given option was set in the outer scope, while being multi-config
# aware.
# It checks e.g. if either arg_RUNTIME_PATH or arg_RUNTIME_PATH_DEBUG is set in the outer scope.
function(_qt_internal_sbom_get_and_check_multi_config_aware_single_arg_option
        arg_prefix arg_name config out_var)
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)

    # Prefer the multi config option if it is set.
    if(is_multi_config)
        string(TOUPPER ${config} config_upper)
        set(outer_scope_var_name "${arg_prefix}_${arg_name}_${config_upper}")
        set(option_name "${arg_name}_${config_upper}")
    endif()

    if(NOT is_multi_config OR NOT DEFINED ${outer_scope_var_name})
        set(outer_scope_var_name "${arg_prefix}_${arg_name}")
        set(option_name "${arg_name}")
    endif()

    if(NOT DEFINED ${outer_scope_var_name})
        message(FATAL_ERROR "Missing ${option_name}")
    endif()

    set(${out_var} "${${outer_scope_var_name}}" PARENT_SCOPE)
endfunction()

# Checks if given config is not the first config in a multi-config build, and thus file installation
# for that config should be optional.
function(_qt_internal_sbom_is_config_optional_in_multi_config config out_var)
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)

    if(QT_MULTI_CONFIG_FIRST_CONFIG)
        set(first_config_type "${QT_MULTI_CONFIG_FIRST_CONFIG}")
    elseif(CMAKE_CONFIGURATION_TYPES)
        list(GET CMAKE_CONFIGURATION_TYPES 0 first_config_type)
    endif()

    if(is_multi_config AND NOT (cmake_config STREQUAL first_config_type))
        set(is_optional TRUE)
    else()
        set(is_optional FALSE)
    endif()

    set(${out_var} "${is_optional}" PARENT_SCOPE)
endfunction()

# Checks if given config is not the first config in a multi-config build, and thus file installation
# for that config should be optional, sets the actual option name.
function(_qt_internal_sbom_get_target_file_is_optional_in_multi_config config out_var)
    _qt_internal_sbom_is_config_optional_in_multi_config("${config}" is_optional)

    if(is_optional)
        set(option "OPTIONAL")
    else()
        set(option "")
    endif()

    set(${out_var} "${option}" PARENT_SCOPE)
endfunction()

# Get a sanitized spdx id for a file.
# For consistency, we prefix the id with SPDXRef-PackagedFile-. This is not a requirement.
function(_qt_internal_sbom_get_file_spdx_id target out_var)
    _qt_internal_sbom_get_spdx_id_unique_suffix(spdx_id_unique_suffix)
    _qt_internal_sbom_get_sanitized_spdx_id(spdx_id
        "SPDXRef-PackagedFile-${target}${spdx_id_unique_suffix}")
    set(${out_var} "${spdx_id}" PARENT_SCOPE)
endfunction()
