# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(__qt_internal_get_supported_min_cmake_version_for_using_qt out_var)
    # This is recorded in Qt6ConfigExtras.cmake
    set(supported_version "${QT_SUPPORTED_MIN_CMAKE_VERSION_FOR_USING_QT}")
    set(${out_var} "${supported_version}" PARENT_SCOPE)
endfunction()

function(__qt_internal_get_computed_min_cmake_version_for_using_qt out_var)
    __qt_internal_force_allow_unsuitable_cmake_version_for_using_qt(allow_any_version)

    # An explicit override for those that take it upon themselves to fix the build system
    # when using a CMake version lower than the one officially supported.
    # Also useful for build testing locally with different minimum versions to observe different
    # policy behaviors.
    if(allow_any_version)
        # Just set some low version, the exact value is not really important.
        set(computed_min_version "3.16")

    # Allow override when configuring user project.
    elseif(QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT)
        set(computed_min_version "${QT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT}")

    # Set in QtConfigExtras.cmake.
    elseif(QT_COMPUTED_MIN_CMAKE_VERSION_FOR_USING_QT)
        set(computed_min_version "${QT_COMPUTED_MIN_CMAKE_VERSION_FOR_USING_QT}")
    else()
        message(FATAL_ERROR
            "Qt Developer error: Can't compute the minimum CMake version required to use this Qt.")
    endif()

    set(${out_var} "${computed_min_version}" PARENT_SCOPE)
endfunction()

function(__qt_internal_warn_if_min_cmake_version_not_met)
    __qt_internal_get_supported_min_cmake_version_for_using_qt(min_supported_version)
    __qt_internal_get_computed_min_cmake_version_for_using_qt(computed_min_version)

    if(NOT min_supported_version STREQUAL computed_min_version
            AND computed_min_version VERSION_LESS min_supported_version)
        message(WARNING
               "The minimum required CMake version to use Qt is: '${min_supported_version}'. "
               "You have explicitly chosen to require a lower minimum CMake version: '${computed_min_version}'. "
               "Using Qt with this CMake version is not officially supported. Use at your own risk."
               )
    endif()
endfunction()

# Sets out_var to either TRUE or FALSE if dependning on whether there was a forceful request to
# allow using the current cmake version to use Qt in projects.
function(__qt_internal_force_allow_unsuitable_cmake_version_for_using_qt out_var)
    set(allow_any_version FALSE)

    # Temporarily allow any version when using Qt in Qt's CI, so we can decouple the provisioning
    # of the minimum CMake version from the bump of the minimum CMake version.
    # The COIN_UNIQUE_JOB_ID env var is set in Qt's CI for both build and test work items.
    # Current state is that this check is disabled.
    set(allow_any_version_in_ci FALSE)

    if(allow_any_version_in_ci AND DEFINED ENV{COIN_UNIQUE_JOB_ID})
        set(allow_any_version TRUE)
    endif()

    # An explicit opt in, for using any CMake version.
    if(QT_FORCE_ANY_CMAKE_VERSION_FOR_USING_QT)
        set(allow_any_version TRUE)
    endif()

    set(${out_var} "${allow_any_version}" PARENT_SCOPE)
endfunction()

function(__qt_internal_require_suitable_cmake_version_for_using_qt)
    # Skip the public project check if we're building a Qt repo because it's too early to do
    # it at find_package(Qt6) time.
    # Instead, a separate check is done in qt_build_repo_begin.
    # We detect a Qt repo by the presence of the QT_REPO_MODULE_VERSION variable set in .cmake.conf
    # of each repo.
    if(QT_REPO_MODULE_VERSION)
        return()
    endif()

    # Only do the setup once per directory scope, because Qt6 is a dependency for many packages,
    # and a recursive call will show the warning multiple times.
    if(__qt_internal_set_up_cmake_minimum_required_version_already_done)
        return()
    endif()
    set(__qt_internal_set_up_cmake_minimum_required_version_already_done TRUE PARENT_SCOPE)

    # Check the overall minimum required CMake version when consuming any Qt CMake package.
    __qt_internal_warn_if_min_cmake_version_not_met()
    __qt_internal_get_computed_min_cmake_version_for_using_qt(computed_min_version)

    if(CMAKE_VERSION VERSION_LESS computed_min_version)
        set(major_minor "${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
        message(FATAL_ERROR
            "CMake ${computed_min_version} or higher is required to use Qt. "
            "You are running version ${CMAKE_VERSION} "
            "Qt requires newer CMake features to work correctly. You can lower the minimum "
            "required version by passing "
            "-DQT_FORCE_MIN_CMAKE_VERSION_FOR_USING_QT=${major_minor} when configuring the "
            "project. Using Qt with this CMake version is not officially supported. "
            "Use at your own risk.")
    endif()
endfunction()

# Handle force-assignment of CMP0156 policy when using CMake 3.29+.
#
# For Apple-platforms we set it to NEW, to avoid duplicate linker issues when using -ObjC flag.
# For Emscripten / WebAssembly we also set it to NEW, to avoid duplicate linker issues.
#
# For other platforms, we leave the policy value as-is, without showing any warnings.
function(__qt_internal_set_cmp0156)
    # Exit early if not using CMake 3.29+
    if(NOT POLICY CMP0156)
        return()
    endif()

    # Honor this variable if it's set and TRUE. It was previously introduced to allow working around
    # the forced OLD value.
    if(QT_FORCE_CMP0156_TO_NEW)
        get_cmake_property(debug_message_shown _qt_internal_cmp0156_debug_message_shown)
        if(NOT debug_message_shown)
            message(DEBUG "Force setting the CMP0156 policy to user provided value: NEW")
            set_property(GLOBAL PROPERTY _qt_internal_cmp0156_debug_message_shown TRUE)
        endif()

        cmake_policy(SET CMP0156 "NEW")
        return()
    endif()

    # Allow forcing to OLD / NEW or empty behavior due the default being NEW for Apple platforms.
    if(QT_FORCE_CMP0156_TO_VALUE)
        cmake_policy(SET CMP0156 "${QT_FORCE_CMP0156_TO_VALUE}")
        message(DEBUG "Force setting the CMP0156 policy to user provided value: "
            "${QT_FORCE_CMP0156_TO_VALUE}")
        return()
    endif()

    # Get the current value of the policy, as saved by the Qt6 package as soon as it is found.
    # We can't just do cmake_policy(GET CMP0156 policy_value) here, because our Qt6Config.cmake and
    # Qt6FooConfig.cmake files use cmake_minimum_required, which reset the policy value that
    # might have been set by the project developer or the user.
    # And a function uses the policy values that were set when the function was defined, not when
    # it is called.
    __qt_internal_get_directory_scope_policy_cmp0156(policy_value)

    # Apple linkers (legacy Apple ld64, as well as the new ld-prime) don't care about the
    # link line order when linking static libraries, compared to Linux GNU ld.
    # But they care about duplicate static frameworks (not libraries) when used in conjunction with
    # the -ObjC flag, which force loads all static libraries / frameworks that contain Objective-C
    # categories or classes. This can cause duplicate symbol errors.
    # To avoid the issue, we want to enable the policy, so we de-duplicate the libraries.
    if(APPLE)
        set(default_policy_value NEW)
        set(unsupported_policy_value OLD)
    elseif(EMSCRIPTEN OR WASM)
        # It's a similar story for Emscripten WebAssembly. If a project passes -sMAIN_MODULE=1,
        # this will cause duplicate symbol errors when qt's static libraries are mentioned
        # more than once on the link line, unless we let CMake deduplicate them. This only works
        # for CMake 4.2+, which lets CMake know that the emscripten linker can resolve all symbols
        # even if the library is not repeated.
        set(default_policy_value NEW)
        set(unsupported_policy_value OLD)
    else()
        # For other platforms we don't enforce any policy values and keep them as-is.
        set(default_policy_value "")
        set(unsupported_policy_value "")
    endif()

    # Force set the default policy value for the given platform, even if the policy value is
    # the same or empty. That's because in the calling function scope, the value can be empty
    # due to the cmake_minimum_required call in Qt6Config.cmake resetting the policy value.
    if(default_policy_value)
        get_cmake_property(debug_message_shown _qt_internal_cmp0156_debug_message_shown)
        if(NOT debug_message_shown)
            message(DEBUG "Force setting the CMP0156 policy to '${default_policy_value}' "
                "for platform '${CMAKE_SYSTEM_NAME}'.")
            set_property(GLOBAL PROPERTY _qt_internal_cmp0156_debug_message_shown TRUE)
        endif()

        cmake_policy(SET CMP0156 "${default_policy_value}")
    endif()

    # If the policy is explicitly set to a value other than the (non-empty) default, issue a
    # warning.
    # Don't show the warning if the policy is unset, which would be the default for most
    # projects, because it's too much noise. Also don't show it for Qt builds.
    if(unsupported_policy_value
            AND "${policy_value}" STREQUAL "${unsupported_policy_value}"
            AND NOT QT_BUILDING_QT)
        message(WARNING
            "CMP0156 is set to '${policy_value}'. Qt forces the '${default_policy_value}'"
            " behavior of this policy for the '${CMAKE_SYSTEM_NAME}' platform by default."
            " Set QT_FORCE_CMP0156_TO_VALUE=${unsupported_policy_value} to force"
            " the '${unsupported_policy_value}' behavior for Qt commands that create"
            " library or executable targets.")
    endif()
endfunction()
