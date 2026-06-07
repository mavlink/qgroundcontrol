# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Wraps install() command. In a prefix build, simply passes along arguments to install().
# In a non-prefix build, handles association of targets to export names, and also calls export().
function(qt_install)
    set(flags)
    set(options EXPORT DESTINATION NAMESPACE)
    set(multiopts TARGETS)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    if(arg_TARGETS)
        set(is_install_targets TRUE)
    endif()

    # In a prefix build, always invoke install() without modification.
    # In a non-prefix build, pass install(TARGETS) commands to allow
    # association of targets to export names, so we can later use the export names
    # in export() commands.
    if(QT_WILL_INSTALL OR is_install_targets)
        install(${ARGV})
    endif()

    # When install(EXPORT) is called, also call export(EXPORT)
    # to generate build tree target files.
    if(NOT is_install_targets AND arg_EXPORT)
        # For prefixed builds (both top-level and per-repo) export build tree CMake Targets files so
        # they can be used in CMake ExternalProjects. One such case is examples built as
        # ExternalProjects as part of the Qt build.
        # In a top-level build the exported config files are placed under qtbase/lib/cmake.
        # In a per-repo build, they will be placed in each repo's build dir/lib/cmake.
        if(QT_WILL_INSTALL)
            qt_path_join(arg_DESTINATION "${QT_BUILD_DIR}" "${arg_DESTINATION}")
        endif()

        set(namespace_option "")
        if(arg_NAMESPACE)
            set(namespace_option NAMESPACE ${arg_NAMESPACE})
        endif()
        export(EXPORT ${arg_EXPORT}
               ${namespace_option}
               FILE "${arg_DESTINATION}/${arg_EXPORT}.cmake")
    endif()
endfunction()

# Copies files using file(COPY) signature in non-prefix builds.
function(qt_non_prefix_copy)
    if(NOT QT_WILL_INSTALL)
        file(${ARGV})
    endif()
endfunction()

# Retrieve the permissions that are set by install(PROGRAMS).
function(qt_get_install_executable_permissions out_var)
    set(default_permissions ${CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS})
    if(NOT default_permissions)
        set(default_permissions OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    endif()
    set(executable_permissions ${default_permissions} OWNER_EXECUTE)
    if(GROUP_READ IN_LIST default_permissions)
        list(APPEND executable_permissions GROUP_EXECUTE)
    endif()
    if(WORLD_READ IN_LIST default_permissions)
        list(APPEND executable_permissions WORLD_EXECUTE)
    endif()
    set(${out_var} ${executable_permissions} PARENT_SCOPE)
endfunction()

# Use case is installing files in a prefix build, or copying them to the correct build dir
# in a non-prefix build.
# Pass along arguments as you would pass them to install().
# Only supports FILES, PROGRAMS and DIRECTORY signature, and without fancy things
# like OPTIONAL or RENAME or COMPONENT.
function(qt_copy_or_install)
    set(flags FILES PROGRAMS DIRECTORY)
    set(options)
    set(multiopts)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    # Remember which option has to be passed to the install command.
    set(copy_arguments "")
    set(argv_copy ${ARGV})
    if(arg_FILES)
        set(install_option "FILES")
    elseif(arg_PROGRAMS)
        set(install_option "PROGRAMS")
        qt_get_install_executable_permissions(executable_permissions)
        list(APPEND copy_arguments FILE_PERMISSIONS ${executable_permissions})
    elseif(arg_DIRECTORY)
        set(install_option "DIRECTORY")
    endif()

    list(REMOVE_AT argv_copy 0)
    qt_install(${install_option} ${argv_copy})
    qt_non_prefix_copy(COPY ${argv_copy} ${copy_arguments})
endfunction()

# Create a versioned hard-link for the given target, or a program
# E.g. "bin/qmake6" -> "bin/qmake".
#
# One-value Arguments:
#     WORKING_DIRECTORY
#         The directory where the original file is already placed.
#     SUFFIX
#         The program file extension, only used for PROGRAMS
# Multi-value Arguments:
#     TARGETS
#         List of targets for which the versioned link will be created.
#         If targets are given, BASE_NAME and SUFFIX will be derived from it.
#     PROGRAMS
#         List of program file names for which the versioned link will be created.
#
#
# NOTE: This assumes that TARGETS, or PROGRAMS are already installed in the
#       WORKING_DIRECTORY.
#
# In a multi-config build, create the link for the main config only.
function(qt_internal_install_versioned_link)
    if(NOT QT_WILL_INSTALL)
        return()
    endif()

    if(NOT QT_CREATE_VERSIONED_HARD_LINK)
        return()
    endif()

    set(options)
    set(oneValueArgs "WORKING_DIRECTORY;SUFFIX")
    set(multiValueArgs "TARGETS;PROGRAMS")
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(arg_TARGETS)
        foreach(target "${arg_TARGETS}")
            _qt_internal_create_versioned_link_or_copy("${arg_WORKING_DIRECTORY}"
                $<TARGET_FILE_BASE_NAME:${target}>
                $<TARGET_FILE_SUFFIX:${target}>)
        endforeach()
    endif()

    if(arg_PROGRAMS)
        foreach(program "${arg_PROGRAMS}")
            _qt_internal_create_versioned_link_or_copy("${arg_WORKING_DIRECTORY}"
                "${program}"
                "${arg_SUFFIX}")
        endforeach()
    endif()
endfunction()

# Generate a script for creating a hard-link between the base_name, and
# base_name${PROJECT_VERSION_MAJOR}.
#
# If no hard link can be created, make a copy instead.
function(_qt_internal_create_versioned_link_or_copy install_dir base_name suffix)
    qt_path_join(install_base_file_path "$\{qt_full_install_prefix}"
        "${install_dir}" "${base_name}")
    set(original "${install_base_file_path}${suffix}")
    set(linkname "${install_base_file_path}${PROJECT_VERSION_MAJOR}${suffix}")
    set(code "set(qt_full_install_prefix \"$\{CMAKE_INSTALL_PREFIX}\")"
        "  if(NOT \"$ENV\{DESTDIR}\" STREQUAL \"\")"
        )

    if(CMAKE_HOST_WIN32)
        list(APPEND code
            "    if(qt_full_install_prefix MATCHES \"^[a-zA-Z]:\")"
            "        string(SUBSTRING \"$\{qt_full_install_prefix}\" 2 -1 qt_full_install_prefix)"
            "    endif()"
            )
    endif()
    list(APPEND code
        "    string(PREPEND qt_full_install_prefix \"$ENV\{DESTDIR}\")"
        "  endif()"
        "  message(STATUS \"Creating hard link ${original} -> ${linkname}\")"
        "  file(CREATE_LINK \"${original}\" \"${linkname}\" COPY_ON_ERROR)")

    if(QT_GENERATOR_IS_MULTI_CONFIG)
        # Wrap the code in a configuration check,
        # because install(CODE) does not support a CONFIGURATIONS argument.
        qt_create_case_insensitive_regex(main_config_regex ${QT_MULTI_CONFIG_FIRST_CONFIG})
        list(PREPEND code "if(\"\${CMAKE_INSTALL_CONFIG_NAME}\" MATCHES \"${main_config_regex}\")")
        list(APPEND code "endif()")
    endif()

    list(JOIN code "\n" code)
    install(CODE "${code}")
endfunction()

# Use case is copying files or directories in a non-prefix build with each build, so that changes
# are available each time, this is useful for some Android templates that are needed for building,
# apks and need to sync changes each time a build is started
function(qt_internal_copy_at_build_time)
    set(flags)
    set(options TARGET DESTINATION)
    set(multiopts FILES DIRECTORIES)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    file(MAKE_DIRECTORY "${arg_DESTINATION}")

    unset(outputs)
    foreach(dir_to_copy IN LISTS arg_DIRECTORIES)
        get_filename_component(file_name "${dir_to_copy}" NAME)
        set(destination_file_name "${arg_DESTINATION}/${file_name}")

        file(GLOB_RECURSE all_files_in_dir RELATIVE "${dir_to_copy}" "${dir_to_copy}/*")
        set(dir_outputs ${all_files_in_dir})
        set(dir_deps ${all_files_in_dir})

        list(TRANSFORM dir_outputs PREPEND "${destination_file_name}/")
        list(TRANSFORM dir_deps PREPEND "${dir_to_copy}/")

        add_custom_command(OUTPUT ${dir_outputs}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${dir_to_copy} "${destination_file_name}"
            DEPENDS ${dir_deps}
            COMMENT "Copying directory ${dir_to_copy} to ${arg_DESTINATION}."
        )
        list(APPEND outputs ${dir_outputs})
    endforeach()

    unset(file_outputs)
    unset(files_to_copy)
    foreach(path_to_copy IN LISTS arg_FILES)
        get_filename_component(file_name "${path_to_copy}" NAME)
        set(destination_file_name "${arg_DESTINATION}/${file_name}")

        list(APPEND file_outputs "${destination_file_name}")
        list(APPEND files_to_copy "${path_to_copy}")
    endforeach()

    if(files_to_copy)
        add_custom_command(OUTPUT ${file_outputs}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${files_to_copy} ${arg_DESTINATION}
            DEPENDS ${files_to_copy}
            COMMENT "Copying files ${files_to_copy} to ${arg_DESTINATION}."
        )
        list(APPEND outputs ${file_outputs})
    endif()

    get_property(count GLOBAL PROPERTY _qt_internal_copy_at_build_time_count)
    if(NOT count)
        set(count 0)
    endif()

    add_custom_target(qt_internal_copy_at_build_time_${count} DEPENDS ${outputs})
    if(arg_TARGET)
        add_dependencies(${arg_TARGET} qt_internal_copy_at_build_time_${count})
    endif()

    math(EXPR count "${count} + 1")
    set_property(GLOBAL PROPERTY _qt_internal_copy_at_build_time_count ${count})
endfunction()
