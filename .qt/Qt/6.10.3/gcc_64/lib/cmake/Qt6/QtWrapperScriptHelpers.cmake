# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Creates and installs the following wrapper CMake scripts:
# qt-make
# qt-cmake-private
# qt-configure-module
# qt-cmake-private-install
# And other helper scripts.
function(qt_internal_create_wrapper_scripts)
    # Provide a convenience cmake wrapper.

    if(QT_GENERATE_WRAPPER_SCRIPTS_FOR_ALL_HOSTS)
        set(generate_unix TRUE)
        set(generate_non_unix TRUE)
    elseif(CMAKE_HOST_UNIX)
        set(generate_unix TRUE)
    else()
        set(generate_non_unix TRUE)
    endif()

    set(extra_qt_cmake_code "")
    if(generate_unix)

        if(UIKIT)
            set(extra_qt_cmake_code [=[
# Specify Xcode as the default generator by assigning it to the CMAKE_GENERATOR env var.
# An explicit -G or -D CMAKE_GENERATOR given on the command line will still take precedence.
export CMAKE_GENERATOR=Xcode
]=])
        endif()

        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake.in"
                       "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake" @ONLY
                       NEWLINE_STYLE LF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake"
                   DESTINATION "${INSTALL_BINDIR}")
    endif()
    if(generate_non_unix)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake.bat.in"
                       "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake.bat" @ONLY
                       NEWLINE_STYLE CRLF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake.bat"
                   DESTINATION "${INSTALL_BINDIR}")
    endif()

    if(generate_unix)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake-create.in"
                    "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-create" @ONLY
                    NEWLINE_STYLE LF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-create"
                DESTINATION "${INSTALL_BINDIR}")
    endif()
    if(generate_non_unix)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake-create.bat.in"
                    "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-create.bat" @ONLY
                    NEWLINE_STYLE CRLF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-create.bat"
                DESTINATION "${INSTALL_BINDIR}")
    endif()

    # Reset the contents for the next script.
    set(extra_qt_cmake_code "")

    # Provide a private convenience wrapper with options that should not be propagated via the
    # public qt-cmake wrapper e.g. CMAKE_GENERATOR.
    # These options can not be set in a toolchain file, but only on the command line.
    # These options should not be in the public wrapper, because a consumer of Qt might want to
    # build their CMake app with the Unix Makefiles generator, while Qt should be built with the
    # Ninja generator. In a similar vein, we do want to use the same compiler for all Qt modules,
    # but not for user applications.
    # The private wrapper is more convenient for building Qt itself, because a developer doesn't
    # need to specify the same options for each qt module built.
    set(__qt_cmake_extra "-G\"${CMAKE_GENERATOR}\" -DQT_USE_ORIGINAL_COMPILER=ON")
    if(generate_unix)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake.in"
            "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/qt-cmake-private" @ONLY
            NEWLINE_STYLE LF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/qt-cmake-private"
               DESTINATION "${INSTALL_LIBEXECDIR}")
    endif()
    if(generate_non_unix)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake.bat.in"
            "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-private.bat" @ONLY
            NEWLINE_STYLE CRLF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-cmake-private.bat"
               DESTINATION "${INSTALL_BINDIR}")
    endif()
    unset(__qt_cmake_extra)

    # Provide a script to configure Qt modules.
    if(QT_WILL_INSTALL)
        set(__relative_path_to_cmake_scripts_dir
            "${__GlobalConfig_relative_path_from_bin_dir_to_cmake_config_dir}")
    else()
        file(RELATIVE_PATH __relative_path_to_cmake_scripts_dir
            "${__qt_bin_dir_absolute}" "${CMAKE_CURRENT_LIST_DIR}")
    endif()
    file(TO_NATIVE_PATH "${__relative_path_to_cmake_scripts_dir}"
        __relative_path_to_cmake_scripts_dir)
    if(generate_unix)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-configure-module.in"
            "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-configure-module" @ONLY
            NEWLINE_STYLE LF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-configure-module"
            DESTINATION "${INSTALL_BINDIR}")
    endif()
    if(generate_non_unix)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-configure-module.bat.in"
            "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-configure-module.bat" @ONLY
            NEWLINE_STYLE CRLF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/qt-configure-module.bat"
            DESTINATION "${INSTALL_BINDIR}")
    endif()
    unset(__relative_path_to_cmake_scripts_dir)

    # Provide a private convenience wrapper to configure and build one or more standalone tests.
    # Calling CMake directly on a Qt test project won't work because the project does not call
    # find_package(Qt...) to get all dependencies like examples do.
    # Instead a template CMakeLists.txt project is used which sets up all the necessary private bits
    # and then calls add_subdirectory on the provided project path.
    set(__qt_cmake_standalone_test_name "qt-cmake-standalone-test")
    if(generate_unix)
        set(__qt_cmake_standalone_test_libexec_path
            "${INSTALL_LIBEXECDIR}/${__qt_cmake_standalone_test_name}")
    endif()
    if(generate_non_unix)
        set(__qt_cmake_standalone_test_bin_path
            "${INSTALL_BINDIR}/${__qt_cmake_standalone_test_name}")
    endif()

    # Configuring a standalone test on iOS should use the Xcode generator, but qt-cmake-private uses
    # the generator that was used to build Qt itself (e.g. Ninja).
    # Use qt-cmake instead, which does use the Xcode generator since Qt 6.2.5, 6.3.1, 6.4.
    if(IOS)
        set(__qt_cmake_private_path
            "${QT_STAGING_PREFIX}/${INSTALL_BINDIR}/qt-cmake")
    else()
        if(generate_unix)
            set(__qt_cmake_private_path
                "${QT_STAGING_PREFIX}/${INSTALL_LIBEXECDIR}/qt-cmake-private")
        endif()
        if(generate_non_unix)
            set(__qt_cmake_private_path
                "${QT_STAGING_PREFIX}/${INSTALL_BINDIR}/qt-cmake-private")
        endif()
    endif()

    set(__qt_cmake_standalone_test_path
        "${__build_internals_install_dir}/${__build_internals_standalone_test_template_dir}")

    if(QT_WILL_INSTALL)
        # Need to prepend the staging prefix when doing prefix builds, because the build internals
        # install dir is relative in that case..
        qt_path_join(__qt_cmake_standalone_test_path
                    "${QT_STAGING_PREFIX}"
                    "${__qt_cmake_standalone_test_path}")
    endif()

    if(generate_unix)
        get_filename_component(rel_base_path
            "${QT_STAGING_PREFIX}/${__qt_cmake_standalone_test_libexec_path}"
            DIRECTORY)

        file(RELATIVE_PATH __qt_cmake_private_relpath "${rel_base_path}"
            "${__qt_cmake_private_path}")
        file(RELATIVE_PATH __qt_cmake_standalone_test_relpath "${rel_base_path}"
            "${__qt_cmake_standalone_test_path}")

        set(__qt_cmake_standalone_test_os_prelude "#!/bin/sh")
        set(__qt_cmake_standalone_test_script_relpath "SCRIPT_DIR=`dirname $0`")
        string(PREPEND __qt_cmake_private_relpath "exec $SCRIPT_DIR/")
        string(PREPEND __qt_cmake_standalone_test_relpath "$SCRIPT_DIR/")
        set(__qt_cmake_standalone_passed_args "\"$@\" -DPWD=\"$PWD\"")

        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake-standalone-test.in"
            "${QT_BUILD_DIR}/${__qt_cmake_standalone_test_libexec_path}"
            NEWLINE_STYLE LF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${__qt_cmake_standalone_test_libexec_path}"
                   DESTINATION "${INSTALL_LIBEXECDIR}")
    endif()
    if(generate_non_unix)
        get_filename_component(rel_base_path
            "${QT_STAGING_PREFIX}/${__qt_cmake_standalone_test_bin_path}"
            DIRECTORY)

        file(RELATIVE_PATH __qt_cmake_private_relpath "${rel_base_path}"
            "${__qt_cmake_private_path}")
        file(RELATIVE_PATH __qt_cmake_standalone_test_relpath "${rel_base_path}"
            "${__qt_cmake_standalone_test_path}")

        set(__qt_cmake_standalone_test_os_prelude "@echo off")
        set(__qt_cmake_standalone_test_script_relpath "set SCRIPT_DIR=%~dp0")
        string(APPEND __qt_cmake_standalone_test_bin_path ".bat")
        string(APPEND __qt_cmake_private_relpath ".bat")
        string(PREPEND __qt_cmake_private_relpath "%SCRIPT_DIR%")
        string(PREPEND __qt_cmake_standalone_test_relpath "%SCRIPT_DIR%")
        set(__qt_cmake_standalone_passed_args "%* -DPWD=\"%CD%\"")

        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/qt-cmake-standalone-test.in"
            "${QT_BUILD_DIR}/${__qt_cmake_standalone_test_bin_path}"
            NEWLINE_STYLE CRLF)
        qt_install(PROGRAMS "${QT_BUILD_DIR}/${__qt_cmake_standalone_test_bin_path}"
                   DESTINATION "${INSTALL_BINDIR}")
    endif()

    # Create an installation script that the CI can use to handle installation for both
    # single and multiple configurations.
    set(__qt_cmake_install_script_name "qt-cmake-private-install.cmake")
    if(CMAKE_CONFIGURATION_TYPES)
        set(__qt_configured_configs "${CMAKE_CONFIGURATION_TYPES}")
    elseif(CMAKE_BUILD_TYPE)
        set(__qt_configured_configs "${CMAKE_BUILD_TYPE}")
    endif()

    if(
        # Skip stripping pure debug builds so it's easier to debug issues in CI VMs.
        (NOT QT_FEATURE_debug_and_release
            AND QT_FEATURE_debug
            AND NOT QT_FEATURE_separate_debug_info)

        # Skip stripping on MSVC because ${CMAKE_STRIP} might contain a MinGW strip binary
        # and the breaks the linker version flag embedded in the binary and causes Qt Creator
        # to mis-identify the Kit ABI.
        OR MSVC
        )
        set(__qt_skip_strip_installed_artifacts TRUE)
    else()
        set(__qt_skip_strip_installed_artifacts FALSE)
    endif()
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/bin/${__qt_cmake_install_script_name}.in"
                   "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/${__qt_cmake_install_script_name}" @ONLY)
    qt_install(FILES "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/${__qt_cmake_install_script_name}"
               DESTINATION "${INSTALL_LIBEXECDIR}")

    qt_internal_create_qt_configure_part_wrapper_script("STANDALONE_TESTS")
    qt_internal_create_qt_configure_part_wrapper_script("STANDALONE_EXAMPLES")
    qt_internal_create_cyclone_dx_sbom_generator_script()

    if(NOT CMAKE_CROSSCOMPILING)
        qt_internal_create_qt_android_runner_wrapper_script()
    endif()
endfunction()

function(qt_internal_create_qt_configure_part_wrapper_script component)
    if(QT_GENERATE_WRAPPER_SCRIPTS_FOR_ALL_HOSTS)
        set(generate_unix TRUE)
        set(generate_non_unix TRUE)
    elseif(CMAKE_HOST_UNIX)
        set(generate_unix TRUE)
    else()
        set(generate_non_unix TRUE)
    endif()

    # Create a private wrapper script to configure and build all standalone tests / examples.
    #
    # The script uses qt-cmake instead of qt-cmake-private on purpose. That's to ensure we build
    # only one configuration of tests (e.g RelWithDebInfo only) when Qt is configured with more
    # than one configuration (RelWithDebInfo;Debug).
    # Meant to be used by our CI instructions.
    #
    # The script takes a path to the repo for which the standalone tests / examples will be
    # configured.

    if(component STREQUAL "STANDALONE_TESTS")
        set(script_name "qt-internal-configure-tests")
        set(script_passed_args "-DQT_BUILD_STANDALONE_TESTS=ON -DQT_BUILD_EXAMPLES=OFF")
    elseif(component STREQUAL "STANDALONE_EXAMPLES")
        set(script_name "qt-internal-configure-examples")
        set(script_passed_args "-DQT_BUILD_STANDALONE_EXAMPLES=ON -DQT_BUILD_TESTS=OFF")
    else()
        message(FATAL_ERROR "Invalid component type: ${component}")
    endif()

    string(APPEND script_passed_args " -DQT_USE_ORIGINAL_COMPILER=ON")

    file(RELATIVE_PATH relative_path_from_libexec_dir_to_bin_dir
        ${__qt_libexec_dir_absolute}
        ${__qt_bin_dir_absolute})
    file(TO_NATIVE_PATH "${relative_path_from_libexec_dir_to_bin_dir}"
                        relative_path_from_libexec_dir_to_bin_dir)

    if(generate_unix)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libexec/${script_name}.in"
            "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/${script_name}" @ONLY
            NEWLINE_STYLE LF)

        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_LIBEXECDIR}/${script_name}"
                   DESTINATION "${INSTALL_LIBEXECDIR}")
    endif()
    if(generate_non_unix)
        configure_file("${CMAKE_CURRENT_SOURCE_DIR}/libexec/${script_name}.bat.in"
            "${QT_BUILD_DIR}/${INSTALL_BINDIR}/${script_name}.bat" @ONLY
            NEWLINE_STYLE CRLF)

        qt_install(PROGRAMS "${QT_BUILD_DIR}/${INSTALL_BINDIR}/${script_name}.bat"
                   DESTINATION "${INSTALL_BINDIR}")
    endif()
endfunction()

# Create a shell wrapper script to reconfigure Qt with the original configure arguments and
# any additional ones passed.
#
# Removes CMakeCache.txt and friends, either manually, or using CMake's --fresh.
#
# The script is created in the root of the build dir and is called config.redo
# It has the same contents as the 'config.status' script we created in qt 5.
function(qt_internal_create_qt_configure_redo_script)
    # Protect against creating the script once per repo in a top level build. Only one file should
    # be created.
    get_cmake_property(script_created _qt_configure_redo_script_created)
    if(script_created)
        return()
    endif()

    set(wrapper_extension "")

    if(NOT CMAKE_HOST_UNIX)
        set(wrapper_extension ".bat")
    endif()

    set(script_name "qt-internal-config.redo")

    set(wrapper_rel_path "${script_name}${wrapper_extension}.in")

    # Need to find the input file depending whether the qtbase sources are available.
    # This mirrors the logic in qt_set_up_build_internals_paths.
    # TODO: Clean this up, together with qt_set_up_build_internals_paths to only use the
    # the qtbase sources when building qtbase. And perhaps also when doing a non-prefix
    # developer-build.
    set(qtbase_in_path "${QT_SOURCE_TREE}/cmake/${wrapper_rel_path}")
    set(installed_in_path
        "${_qt_cmake_dir}/${QT_CMAKE_EXPORT_NAMESPACE}/${wrapper_rel_path}")

    # qtbase sources available, always use them, regardless of prefix or non-prefix builds.
    if(EXISTS "${qtbase_in_path}")
        set(input_script_path "${qtbase_in_path}")

    # qtbase sources unavailable, use installed files.
    elseif(EXISTS "${installed_in_path}")
        set(input_script_path "${installed_in_path}")
    else()
        message(FATAL_ERROR "Can't find ${script_name}${wrapper_extension}.in file.")
    endif()

    # We don't use QT_BUILD_DIR because we want the file in the root of the build dir in a top-level
    # build.
    set(output_script_name "config.redo")
    set(output_path "${CMAKE_BINARY_DIR}/${output_script_name}${wrapper_extension}")

    set(repo_path "")
    if(QT_SUPERBUILD)
        set(configure_script_path "${Qt_SOURCE_DIR}/configure")
    elseif(QtBase_SOURCE_DIR)
        set(configure_script_path "${QtBase_SOURCE_DIR}/configure")
    else()
        if(QT_WILL_INSTALL)
            set(configure_script_path "${QT_STAGING_PREFIX}")
        else()
            set(configure_script_path "${QT_BUILD_DIR}")
        endif()

        string(APPEND configure_script_path
            "/${INSTALL_BINDIR}/qt-configure-module${wrapper_extension}")

        # When configuring a repo other than qtbase, we also need to provide the location
        # to the repo sources.
        set(repo_path "${CMAKE_SOURCE_DIR}")
    endif()

    # Used in the file contents.
    file(TO_NATIVE_PATH "${configure_script_path}" configure_path)

    if(CMAKE_HOST_UNIX)
        set(newline_style "LF")
    else()
        set(newline_style "CRLF")
    endif()

    configure_file("${input_script_path}" "${output_path}" @ONLY NEWLINE_STYLE ${newline_style})

    set_property(GLOBAL PROPERTY _qt_configure_redo_script_created TRUE)
endfunction()

function(qt_internal_create_cyclone_dx_sbom_generator_script)
    _qt_internal_sbom_get_cyclone_dx_generator_script_name(generator_name generator_relative_dir)

    qt_path_join(build_dir_destination "${QT_BUILD_DIR}" "${INSTALL_LIBEXECDIR}")
    qt_path_join(install_destination "${QT_INSTALL_DIR}" "${INSTALL_LIBEXECDIR}")
    qt_path_join(script_path
        "${CMAKE_CURRENT_SOURCE_DIR}" "${generator_relative_dir}" "${generator_name}")

    if(QT_WILL_INSTALL)
        file(COPY "${script_path}"
            DESTINATION "${build_dir_destination}"
        )
    endif()

    qt_copy_or_install(PROGRAMS "${script_path}" DESTINATION "${install_destination}")
endfunction()

function(qt_internal_create_qt_android_runner_wrapper_script)
    qt_path_join(android_runner_destination "${QT_INSTALL_DIR}" "${INSTALL_LIBEXECDIR}")
    qt_path_join(android_runner "${CMAKE_CURRENT_SOURCE_DIR}" "libexec" "qt-android-runner.py")
    qt_copy_or_install(PROGRAMS "${android_runner}" DESTINATION "${android_runner_destination}")
endfunction()
