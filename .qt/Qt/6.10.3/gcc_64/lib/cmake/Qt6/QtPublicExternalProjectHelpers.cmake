# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include_guard()

# Get CMake variables that are needed to build external projects such as examples or CMake test
# projects.
#
# CMAKE_DIR_VAR: Variable name to store the path to the Qt6 CMake config module.
#
# PREFIXES_VAR: Variable name to store the prefixes that can be passed as CMAKE_PREFIX_PATH.
#
# ADDITIONAL_PACKAGES_PREFIXES_VAR: Variable name to store the prefixes that can be appended to
# QT_ADDITIONAL_PACKAGES_PREFIX_PATH.
function(_qt_internal_get_build_vars_for_external_projects)
    set(no_value_options "")
    set(single_value_options
        CMAKE_DIR_VAR
        PREFIXES_VAR
        ADDITIONAL_PACKAGES_PREFIXES_VAR
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    # Standalone tests and examples have QT_BUILD_DIR pointing to the fake standalone prefix.
    # Use instead the relocatable prefix, because qt must have been built / installed by this point.
    if(QT_INTERNAL_BUILD_STANDALONE_PARTS)
        qt_path_join(qt_cmake_dir
            "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}"
            "${INSTALL_LIBDIR}/cmake/${QT_CMAKE_EXPORT_NAMESPACE}"
        )

        set(qt_prefixes "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
        set(qt_additional_packages_prefixes "${qt_prefixes}")

        if(QT_WILL_INSTALL)
            list(APPEND qt_prefixes "${QT6_INSTALL_PREFIX}")
        endif()
    # TODO: Fix example/test builds when using Conan / install prefixes are different for each repo.
    elseif(QT_SUPERBUILD OR QtBase_BINARY_DIR)
        # When doing a top-level build or when building qtbase,
        # always use the Config file from the current build directory, even for prefix builds.
        # We strive to allow building examples without installing Qt first, which means we can't
        # use the install or staging Config files.
        set(qt_prefixes "${QT_BUILD_DIR}")
        set(qt_cmake_dir "${QT_CONFIG_BUILD_DIR}/${QT_CMAKE_EXPORT_NAMESPACE}")
        set(qt_additional_packages_prefixes "")
    else()
        # This is a per-repo build that isn't the qtbase repo, so we know that
        # qtbase was found via find_package() and Qt6_DIR must be set
        set(qt_cmake_dir "${${QT_CMAKE_EXPORT_NAMESPACE}_DIR}")

        # In a prefix build of a non-qtbase repo, we want to pick up the installed Config files
        # for all repos except the one that is currently built. For the repo that is currently
        # built, we pick up the Config files from the current repo build dir instead.
        # For non-prefix builds, there's only one prefix, the main build dir.
        # Both are handled by this assignment.
        set(qt_prefixes "${QT_BUILD_DIR}")

        # Appending to QT_ADDITIONAL_PACKAGES_PREFIX_PATH helps find Qt6 components in
        # non-qtbase prefix builds because we use NO_DEFAULT_PATH in find_package calls.
        # It also handles the cross-compiling scenario where we need to adjust both the root path
        # and prefixes, with the prefixes containing lib/cmake. This leverages the infrastructure
        # previously added for Conan.
        set(qt_additional_packages_prefixes "${qt_prefixes}")

        # In a prefix build, look up all repo Config files in the install prefix,
        # except for the current repo, which will look in the build dir (handled above).
        if(QT_WILL_INSTALL)
            list(APPEND qt_prefixes "${QT6_INSTALL_PREFIX}")
        endif()
    endif()

    if(arg_CMAKE_DIR_VAR)
        set("${arg_CMAKE_DIR_VAR}" "${qt_cmake_dir}" PARENT_SCOPE)
    endif()
    if(arg_PREFIXES_VAR)
        set("${arg_PREFIXES_VAR}" "${qt_prefixes}" PARENT_SCOPE)
    endif()
    if(arg_ADDITIONAL_PACKAGES_PREFIXES_VAR)
        set("${arg_ADDITIONAL_PACKAGES_PREFIXES_VAR}" "${qt_additional_packages_prefixes}"
            PARENT_SCOPE)
    endif()
endfunction()
