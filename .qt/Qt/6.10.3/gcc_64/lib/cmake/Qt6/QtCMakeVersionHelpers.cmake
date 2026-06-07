# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Returns the minimum supported CMake version required to /build/ Qt as originally advertised by Qt.
function(qt_internal_get_supported_min_cmake_version_for_building_qt out_var)
    if(NOT DEFINED BUILD_SHARED_LIBS)
        message(FATAL_ERROR "BUILD_SHARED_LIBS is needed to decide the minimum CMake version. "
            "It should have been set by this point.")
    endif()

    # First check if a value is already set in QtBuildInternalsExtras.cmake, which means we're
    # building a repo other than qtbase and the minimum version was already recorded.
    if(QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT)
        set(supported_version "${QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT}")

    # We're building qtbase so the values come from .cmake.conf.
    elseif(APPLE)
        set(supported_version "${QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT_APPLE}")
    elseif(BUILD_SHARED_LIBS)
        set(supported_version "${QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT_SHARED}")
    else()
        set(supported_version "${QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT_STATIC}")
    endif()

    set(${out_var} "${supported_version}" PARENT_SCOPE)
endfunction()

# Returns the minimum supported CMake version required to /use/ Qt as originally advertised by Qt.
function(qt_internal_get_supported_min_cmake_version_for_using_qt out_var)
    if(NOT DEFINED BUILD_SHARED_LIBS)
        message(FATAL_ERROR "BUILD_SHARED_LIBS is needed to decide the minimum CMake version. "
            "It should have been set by this point.")
    endif()

    if(APPLE)
        set(supported_version "${QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_USING_QT_APPLE}")
    elseif(BUILD_SHARED_LIBS)
        set(supported_version "${QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_USING_QT_SHARED}")
    else()
        set(supported_version "${QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_USING_QT_STATIC}")
    endif()

    set(${out_var} "${supported_version}" PARENT_SCOPE)
endfunction()

# Returns the computed minimum supported CMake version required to /build/ Qt.
function(qt_internal_get_computed_min_cmake_version_for_building_qt out_var)
    qt_internal_force_allow_unsuitable_cmake_version_for_building_qt(allow_any_version)

    # An explicit override for those that take it upon themselves to fix the build system
    # when using a CMake version lower than the one officially supported.
    # Also useful for build testing locally with different minimum versions to observe different
    # policy behaviors.
    if(allow_any_version)
        # Just set some low version, the exact value is not really important.
        set(computed_min_version "3.16")

    # Another explicit override with the same reasoning as above, but with an exact version
    # provided.
    elseif(QT_FORCE_MIN_CMAKE_VERSION_FOR_BUILDING_QT)
        set(computed_min_version "${QT_FORCE_MIN_CMAKE_VERSION_FOR_BUILDING_QT}")

    # Set in QtBuildInternalsExtras.cmake, which means it was already computed as part of qtbase
    # configuration.
    elseif(QT_COMPUTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT)
        set(computed_min_version "${QT_COMPUTED_MIN_CMAKE_VERSION_FOR_BUILDING_QT}")

    # No override was given and the version was not computed before, thus initialize with the
    # default minimum.
    else()
        qt_internal_get_supported_min_cmake_version_for_building_qt(min_supported_version)
        set(computed_min_version "${min_supported_version}")
    endif()
    set(${out_var} "${computed_min_version}" PARENT_SCOPE)
endfunction()

# Returns the computed minimum supported CMake version required to /use/ Qt.
function(qt_internal_get_computed_min_cmake_version_for_using_qt out_var)
    # Allow overriding the required minimum CMake version for user projects, without forcing
    # each project developer to have to override it manually.
    if(QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT)
        set(computed_min_version "${QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT}")

    # No override was given, thus initialize with the default minimum.
    else()
        qt_internal_get_supported_min_cmake_version_for_using_qt(min_supported_version)
        set(computed_min_version "${min_supported_version}")
    endif()
    set(${out_var} "${computed_min_version}" PARENT_SCOPE)
endfunction()

# Returns the oldest CMake version for which NEW policies should be enabled.
# It can be older than the minimum supported or computed CMake version, as it
# is only used for policy settings. The currently running CMake must not be
# older than this version though (doing so will result in an error).
function(qt_internal_get_min_new_policy_cmake_version out_var)
    if(NOT DEFINED BUILD_SHARED_LIBS)
        message(FATAL_ERROR "BUILD_SHARED_LIBS is needed to decide the oldest CMake version "
            "for which NEW policies should be enabled. "
            "It should have been set by this point.")
    endif()

    # First check if a value is already set in QtBuildInternalsExtras.cmake, which means we're
    # building a repo other than qtbase and the lower version was already recorded.
    if(QT_MIN_NEW_POLICY_CMAKE_VERSION)
        set(lower_version "${QT_MIN_NEW_POLICY_CMAKE_VERSION}")

    # We're building qtbase so the values come from .cmake.conf.
    elseif(APPLE)
        set(lower_version "${QT_MIN_NEW_POLICY_CMAKE_VERSION_QT_APPLE}")
    elseif(BUILD_SHARED_LIBS)
        set(lower_version "${QT_MIN_NEW_POLICY_CMAKE_VERSION_QT_SHARED}")
    else()
        set(lower_version "${QT_MIN_NEW_POLICY_CMAKE_VERSION_QT_STATIC}")
    endif()

    set(${out_var} "${lower_version}" PARENT_SCOPE)
endfunction()

# Returns the latest CMake version for which NEW policies should be enabled.
# This cannot be less than the minimum CMake policy version or we will end up
# specifying a version range with the max less than the min.
function(qt_internal_get_max_new_policy_cmake_version out_var)
    if(NOT DEFINED BUILD_SHARED_LIBS)
        message(FATAL_ERROR "BUILD_SHARED_LIBS is needed to decide the latest CMake version "
            "for which NEW policies should be enabled. "
            "It should have been set by this point.")
    endif()

    # First check if a value is already set in QtBuildInternalsExtras.cmake, which means we're
    # building a repo other than qtbase and the lower version was already recorded.
    if(QT_MAX_NEW_POLICY_CMAKE_VERSION)
        set(upper_version "${QT_MAX_NEW_POLICY_CMAKE_VERSION}")

    # We're building qtbase so the values come from .cmake.conf.
    elseif(APPLE)
        set(upper_version "${QT_MAX_NEW_POLICY_CMAKE_VERSION_QT_APPLE}")
    elseif(BUILD_SHARED_LIBS)
        set(upper_version "${QT_MAX_NEW_POLICY_CMAKE_VERSION_QT_SHARED}")
    else()
        set(upper_version "${QT_MAX_NEW_POLICY_CMAKE_VERSION_QT_STATIC}")
    endif()

    qt_internal_get_min_new_policy_cmake_version(lower_version)
    if(upper_version VERSION_LESS lower_version)
        set(upper_version ${lower_version})
    endif()

    set(${out_var} "${upper_version}" PARENT_SCOPE)
endfunction()

function(qt_internal_check_and_warn_about_unsuitable_cmake_version)
    # Don't show the warnings multiple times in a top-level build.
    get_cmake_property(check_done _qt_unsuitable_cmake_version_check_done)
    if(check_done)
        return()
    endif()
    set_property(GLOBAL PROPERTY _qt_unsuitable_cmake_version_check_done TRUE)

    qt_internal_warn_if_min_cmake_version_not_met()
    qt_internal_warn_about_buggy_cmake_versions()
endfunction()

# Sets out_var to either TRUE or FALSE if dependning on whether there was a forceful request to
# allow using the current cmake version to build Qt.
function(qt_internal_force_allow_unsuitable_cmake_version_for_building_qt out_var)
    set(allow_any_version FALSE)

    # Temporarily allow any version when building in Qt's CI, so we can decouple the provisioning
    # of the minimum CMake version from the bump of the minimum CMake version.
    # The COIN_UNIQUE_JOB_ID env var is set in Qt's CI for both build and test work items.
    # Current state is that this check is disabled.
    set(allow_any_version_in_ci FALSE)

    if(allow_any_version_in_ci AND DEFINED ENV{COIN_UNIQUE_JOB_ID})
        set(allow_any_version TRUE)
    endif()

    # An explicit opt in, for using any CMake version.
    if(QT_FORCE_ANY_CMAKE_VERSION_FOR_BUILDING_QT)
        set(allow_any_version TRUE)
    endif()

    set(${out_var} "${allow_any_version}" PARENT_SCOPE)
endfunction()

# Function to be used in downstream repos (like qtsvg) to require a minimum CMake version and warn
# about unsuitable cmake versions.
#
# Such repos don't have the required version information at cmake_minimum_required() time, that's
# why we provide this function to be called at a time when the info is available.
function(qt_internal_require_suitable_cmake_version)
    qt_internal_check_and_warn_about_unsuitable_cmake_version()
    qt_internal_get_computed_min_cmake_version_for_building_qt(computed_min_version)

    if(CMAKE_VERSION VERSION_LESS computed_min_version)
        set(major_minor "${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
        message(FATAL_ERROR
            "CMake ${computed_min_version} or higher is required. "
            "You are running version ${CMAKE_VERSION} "
            "\nQt requires newer CMake features to build correctly. You can lower the minimum "
            "required version by passing "
            "-DQT_FORCE_MIN_CMAKE_VERSION_FOR_BUILDING_QT=${major_minor} when configuring Qt. "
            "Building Qt with this CMake version is not officially supported. Use at your own risk."
            )
    endif()
endfunction()

function(qt_internal_warn_if_min_cmake_version_not_met)
    qt_internal_get_supported_min_cmake_version_for_building_qt(min_supported_version)
    qt_internal_get_computed_min_cmake_version_for_building_qt(computed_min_version)

    if(NOT min_supported_version STREQUAL computed_min_version
            AND computed_min_version VERSION_LESS min_supported_version)
        message(WARNING
               "The minimum required CMake version to build Qt is: '${min_supported_version}'. "
               "You have explicitly chosen to require a lower minimum CMake version: '${computed_min_version}'. "
               "Building Qt with this CMake version is not officially supported. Use at your own risk.")
    endif()
endfunction()

function(qt_internal_warn_about_buggy_cmake_versions)
    set(unsuitable_versions "")

    # Touching a library's source file causes unnecessary rebuilding of unrelated targets.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21020
    list(APPEND unsuitable_versions "3.18.0")

    # Ninja Multi-Config race condition overrides response files of different configurations
    # https://gitlab.kitware.com/cmake/cmake/-/issues/20961
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21050 (follow up issue)
    list(APPEND unsuitable_versions "3.18.1")

    # AUTOMOC dependencies are not properly created when using Ninja Multi-Config.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21118
    #
    # Also until 3.18.2 inclusive, AUTOMOC dependencies are out-of-date if a previously header
    # disappears (for example when upgrading a compiler)
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21136
    #
    # Also multi-arch PCH doesn't work on iOS. Can't quite find the original upstream CMake issue
    # but the Qt one was detected at https://codereview.qt-project.org/c/qt/qt5/+/310947
    # And a follow up issue regarding PCH and -fembed-bitcode failing.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21163
    list(APPEND unsuitable_versions "3.18.2")

    # Cyclic dependencies are created when mixing AUTOMOC/AUTOUIC with sources
    # that have their SKIP_MOC or SKIP_UIC source file properties set to true.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/21977
    list(APPEND unsuitable_versions "3.20.0")

    # AUTOMOC can crash or hang when using a Qt that supports moc depfiles.
    # Issues reported on Windows with Ninja and Makefiles, but it could be happening
    # on other platforms too.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/22014
    list(APPEND unsuitable_versions "3.20.1")

    # Cyclic dependencies can happen when the AUTOMOC / AUTOUIC include directory is added as a
    # target include directory.
    # https://gitlab.kitware.com/cmake/cmake/-/merge_requests/6380
    # https://gitlab.kitware.com/cmake/cmake/-/merge_requests/6359
    # https://gitlab.kitware.com/cmake/cmake/-/issues/16776
    list(APPEND unsuitable_versions "3.21.0")

    # Changing a C++ source file can trigger rebuilds of a lot of other source files that might
    # include AUTOGEN'ed headers or sources.
    # https://gitlab.kitware.com/cmake/cmake/-/issues/22531
    # Fixed in 3.21.2.
    list(APPEND unsuitable_versions "3.21.1")

    foreach(unsuitable_version ${unsuitable_versions})
        if(CMAKE_VERSION VERSION_EQUAL unsuitable_version)
            message(WARNING
                "The CMake version you are using is known to cause issues when building Qt. "
                "Please upgrade to a newer version. "
                "CMake version used: '${unsuitable_version}'")
        endif()
    endforeach()

    # Ninja Multi-Config was introduced in 3.17, but we recommend 3.18.
    set(min_nmc_cmake_version "3.18.3")
    if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config"
            AND CMAKE_VERSION VERSION_LESS ${min_nmc_cmake_version})
        message(WARNING
            "You are using CMake ${CMAKE_VERSION} with the Ninja Multi-Config generator. "
            "This combination is unsupported. "
            "Please upgrade to at least CMake ${min_nmc_cmake_version}. ")
    endif()
endfunction()

# Used to upgrade policies only when building Qt repositories.
#
# Functions don't have their own policy scope, so the policy settings modified
# here will be those of the caller's policy scope. Note that these settings
# will only apply to functions and macros defined after this function is called,
# but not to any that are already defined. Ordinary CMake code not inside a
# function or macro will be affected by these policy settings too.
function(qt_internal_upgrade_cmake_policies)
    qt_internal_get_min_new_policy_cmake_version(lower_version)
    qt_internal_get_max_new_policy_cmake_version(upper_version)
    cmake_minimum_required(VERSION ${lower_version}...${upper_version})
endfunction()

# Get which version to use for CMAKE_POLICY_VERSION_MINIMUM on Android.
# Allow various overrides via QT_ANDROID_CMAKE_POLICY_VERSION_MINIMUM and reading an existing
# CMAKE_POLICY_VERSION_MINIMUM.
function(qt_internal_get_android_cmake_policy_version_minimum_value out_var)
    if(QT_ANDROID_CMAKE_POLICY_VERSION_MINIMUM)
        set(value "${QT_ANDROID_CMAKE_POLICY_VERSION_MINIMUM}")
    elseif(CMAKE_POLICY_VERSION_MINIMUM)
        set(value "${CMAKE_POLICY_VERSION_MINIMUM}")
    else()
        qt_internal_get_android_qt_default_cmake_policy_version_minimum(default_value)
        set(value "${default_value}")
    endif()

    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

# NOTE: If updating the version, also update
# qt_auto_detect_set_android_cmake_policy_version_minimum.
function(qt_internal_get_android_qt_default_cmake_policy_version_minimum out_var)
    set(${out_var} "3.10" PARENT_SCOPE)
endfunction()

# Handle assignment of CMAKE_POLICY_VERSION_MINIMUM for Android NDK cmake toolchain files shipped
# with NDK < r28, to avoid deprecation warnings.
# See https://github.com/android/ndk/issues/2100
# and https://android.googlesource.com/platform/ndk/+/799e5a2d44cc2cc6c7d67f52f2d67957944b7680
# The function is to get the appropriate var asisgnment for try_compile calls,
# as well as writing it to the Qt generated toolchain file.
# Various opt-outs and opt-ins are provided via QT_NO_SET_ANDROID_CMAKE_POLICY_VERSION_MINIMUM
# and QT_ANDROID_CMAKE_POLICY_VERSION_MINIMUM.
#
# See also usage in qt_auto_detect_set_android_cmake_policy_version_minimum.
function(qt_internal_get_android_cmake_policy_version_minimum_assignment out_var)
    set(option_args "")
    set(single_args
        TYPE
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${option_args}"
        "${single_args}"
        "${multi_args}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    set(value "")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0"
            AND NOT QT_NO_SET_ANDROID_CMAKE_POLICY_VERSION_MINIMUM
        )
        qt_internal_get_android_cmake_policy_version_minimum_value(version)

        if(arg_TYPE STREQUAL "COMMAND_LINE")
            set(value "-DCMAKE_POLICY_VERSION_MINIMUM=${version}")

        elseif(arg_TYPE STREQUAL "TOOLCHAIN_FILE_ASSIGNMENT")
            set(value "
# Avoid deprecation warnings in Android ndk cmake toolchain file < r28
set(__qt_initially_configured_android_cmake_policy_version_minimum \"${version}\")
if(CMAKE_VERSION VERSION_GREATER_EQUAL \"4.0\"
        AND NOT QT_NO_SET_ANDROID_CMAKE_POLICY_VERSION_MINIMUM
    )
    if(QT_ANDROID_CMAKE_POLICY_VERSION_MINIMUM)
        set(__qt_toolchain_cmake_policy_version_minimum
            \"\${QT_ANDROID_CMAKE_POLICY_VERSION_MINIMUM}\")
    elseif(CMAKE_POLICY_VERSION_MINIMUM)
        set(__qt_toolchain_cmake_policy_version_minimum
            \"\${CMAKE_POLICY_VERSION_MINIMUM}\")
    else()
        set(__qt_toolchain_cmake_policy_version_minimum
            \"\${__qt_initially_configured_android_cmake_policy_version_minimum}\")
    endif()
    set(CMAKE_POLICY_VERSION_MINIMUM \"\${__qt_toolchain_cmake_policy_version_minimum}\")
    message(DEBUG
        \"Setting CMAKE_POLICY_VERSION_MINIMUM to \"
        \"\${__qt_toolchain_cmake_policy_version_minimum} in toolchain file.\")
endif()
")
        else()
            message(FATAL_ERROR "Unknown TYPE value '${arg_TYPE}'. "
                " Supported values are COMMAND_LINE and TOOLCHAIN_FILE_ASSIGNMENT.")
        endif()
    endif()

    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()
