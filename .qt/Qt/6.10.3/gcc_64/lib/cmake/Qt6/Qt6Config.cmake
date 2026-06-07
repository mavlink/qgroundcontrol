# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was QtConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

# This is included before the cmake_minimum_required on purpose.
include("${CMAKE_CURRENT_LIST_DIR}/QtPublicCMakeEarlyPolicyHelpers.cmake")
__qt_internal_save_directory_scope_policy_cmp0156()

cmake_minimum_required(VERSION 3.16...3.21)

include("${CMAKE_CURRENT_LIST_DIR}/QtPublicCMakeHelpers.cmake")

# ConfigExtras uses functions from QtPublicCMakeHelpers.cmake
include("${CMAKE_CURRENT_LIST_DIR}/Qt6ConfigExtras.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/QtPublicCMakeVersionHelpers.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/QtInstallPaths.cmake")

__qt_internal_require_suitable_cmake_version_for_using_qt()

get_filename_component(_qt_cmake_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_qt_6_config_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

if (NOT QT_NO_CREATE_TARGETS)
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6Targets.cmake")
    if(NOT QT_NO_CREATE_VERSIONLESS_TARGETS)
        if(CMAKE_VERSION VERSION_LESS 3.18 OR QT_USE_OLD_VERSION_LESS_TARGETS)
            include("${CMAKE_CURRENT_LIST_DIR}/Qt6VersionlessTargets.cmake")
        else()
            include("${CMAKE_CURRENT_LIST_DIR}/Qt6VersionlessAliasTargets.cmake")
        endif()
    endif()
else()
    # For examples using `find_package(...)` inside their CMakeLists.txt files:
    # Make CMake's AUTOGEN detect this Qt version properly
    set_directory_properties(PROPERTIES
                             QT_VERSION_MAJOR 6
                             QT_VERSION_MINOR 10
                             QT_VERSION_PATCH 3)
endif()

if(TARGET Qt6::PlatformCommonInternal)
    get_target_property(_qt_platform_internal_common_target
        Qt6::PlatformCommonInternal ALIASED_TARGET)
    if(NOT _qt_platform_internal_common_target)
        set(_qt_platform_internal_common_target Qt6::PlatformCommonInternal)
    endif()
    set(_qt_internal_clang_msvc_frontend FALSE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
        CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(_qt_internal_clang_msvc_frontend TRUE)
    endif()
    set_target_properties(${_qt_platform_internal_common_target}
        PROPERTIES
            _qt_internal_cmake_generator "${CMAKE_GENERATOR}"
            _qt_internal_clang_msvc_frontend "${_qt_internal_clang_msvc_frontend}"
    )
    unset(_qt_platform_internal_common_target)
endif()

get_filename_component(_qt_import_prefix "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(_qt_import_prefix "${_qt_import_prefix}" REALPATH)
list(APPEND CMAKE_MODULE_PATH "${_qt_import_prefix}")
list(APPEND CMAKE_MODULE_PATH "${_qt_import_prefix}/3rdparty/extra-cmake-modules/find-modules")
list(APPEND CMAKE_MODULE_PATH "${_qt_import_prefix}/3rdparty/kwin")

if(APPLE)
    if(NOT CMAKE_SYSTEM_NAME OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(__qt_internal_cmake_apple_support_files_path "${_qt_import_prefix}/macos")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(__qt_internal_cmake_apple_support_files_path "${_qt_import_prefix}/ios")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
        set(__qt_internal_cmake_apple_support_files_path "${_qt_import_prefix}/visionos")
    endif()
endif()

# Public helpers available to all Qt packages.
set(__qt_public_files_to_include
    QtFeature
    QtFeatureCommon
    QtPublicAndroidHelpers
    QtPublicAppleHelpers
    QtPublicCMakeHelpers
    QtPublicCMakeVersionHelpers
    QtPublicDependencyHelpers
    QtPublicExternalProjectHelpers
    QtPublicFinalizerHelpers
    QtPublicFindPackageHelpers
    QtPublicGitHelpers
    QtPublicPluginHelpers
    QtPublicPluginHelpers_v2
    QtPublicSbomAttributionHelpers
    QtPublicSbomBuildToolHelpers
    QtPublicSbomCommonGenerationHelpers
    QtPublicSbomCpeHelpers
    QtPublicSbomCycloneDXHelpers
    QtPublicSbomDocumentNamespaceHelpers
    QtPublicSbomDepHelpers
    QtPublicSbomExternalReferenceHelpers
    QtPublicSbomFileHelpers
    QtPublicSbomGenerationHelpers
    QtPublicSbomGenerationCycloneDXHelpers
    QtPublicSbomHelpers
    QtPublicSbomLicenseHelpers
    QtPublicSbomOpsHelpers
    QtPublicSbomPurlHelpers
    QtPublicSbomPythonHelpers
    QtPublicSbomQtEntityHelpers
    QtPublicSbomRelationshipHelpers
    QtPublicSbomSystemDepHelpers
    QtPublicTargetHelpers
    QtPublicTestHelpers
    QtPublicToolHelpers
    QtPublicWalkLibsHelpers
    QtPublicWindowsHelpers
)
foreach(__qt_public_file_to_include IN LISTS __qt_public_files_to_include)
    include("${__qt_public_file_to_include}")
endforeach()

if(NOT DEFINED QT_CMAKE_EXPORT_NAMESPACE)
    set(QT_CMAKE_EXPORT_NAMESPACE Qt6)
endif()

set(QT_ADDITIONAL_PACKAGES_PREFIX_PATH "" CACHE STRING
    "Additional directories where find(Qt6 ...) components are searched")
set(QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH "" CACHE STRING
    "Additional directories where find(Qt6 ...) host Qt components are searched")

__qt_internal_collect_additional_prefix_paths(_qt_additional_packages_prefix_paths
    QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
__qt_internal_collect_additional_prefix_paths(_qt_additional_host_packages_prefix_paths
    QT_ADDITIONAL_HOST_PACKAGES_PREFIX_PATH)

__qt_internal_prefix_paths_to_roots(_qt_additional_host_packages_root_paths
    "${_qt_additional_host_packages_prefix_paths}")

__qt_internal_collect_additional_module_paths()

# Propagate sanitizer flags to both internal Qt builds and user projects.
# Allow opt-out in case if downstream projects handle it in a different way.
set(QT_CONFIGURED_SANITIZER_OPTIONS "")

if(QT_CONFIGURED_SANITIZER_OPTIONS
   AND NOT __qt_sanitizer_options_set
   AND NOT QT_NO_ADD_SANITIZER_OPTIONS)
    set(ECM_ENABLE_SANITIZERS "${QT_CONFIGURED_SANITIZER_OPTIONS}")
    include(
        "${CMAKE_CURRENT_LIST_DIR}/3rdparty/extra-cmake-modules/modules/ECMEnableSanitizers.cmake")
endif()
# Mark that the current directory scope has its sanitizer flags set.
set(__qt_sanitizer_options_set TRUE)



# Find required dependencies, if any.
include(CMakeFindDependencyMacro)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/Qt6Dependencies.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/Qt6Dependencies.cmake")

    _qt_internal_suggest_dependency_debugging(Qt6
        __qt_Qt6_pkg ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE)

    if(NOT Qt6_FOUND)
        # Clear the components, no need to look for them if dependencies were not found, otherwise
        # you get a wall of recursive error messages.
        set(Qt6_FIND_COMPONENTS "")
    endif()
endif()

set(_Qt6_FIND_PARTS_QUIET)
if(Qt6_FIND_QUIETLY)
     set(_Qt6_FIND_PARTS_QUIET QUIET)
endif()

set(__qt_use_no_default_path_for_qt_packages "NO_DEFAULT_PATH")
if(QT_DISABLE_NO_DEFAULT_PATH_IN_QT_PACKAGES)
    set(__qt_use_no_default_path_for_qt_packages "")
endif()

set(__qt_umbrella_find_components ${Qt6_FIND_COMPONENTS})
__qt_internal_handle_find_all_qt_module_packages(__qt_umbrella_find_components
    COMPONENTS ${__qt_umbrella_find_components}
)

foreach(module ${__qt_umbrella_find_components})
    if(NOT "${QT_HOST_PATH}" STREQUAL ""
       AND "${module}" MATCHES "Tools$"
       AND NOT "${module}" MATCHES "UiTools$"
       AND NOT "${module}" MATCHES "ShaderTools$"
       AND NOT "${module}" MATCHES "^Tools$"
       AND NOT QT_NO_FIND_HOST_TOOLS_PATH_MANIPULATION)
        # Make sure that a Qt*Tools package is also looked up in QT_HOST_PATH.
        # But don't match QtShaderTools and QtTools which are cross-compiled target package names.
        # Allow opt out just in case.
        get_filename_component(__qt_find_package_host_qt_path
            "${Qt6HostInfo_DIR}/.." ABSOLUTE)
        set(__qt_backup_cmake_prefix_path "${CMAKE_PREFIX_PATH}")
        set(__qt_backup_cmake_find_root_path "${CMAKE_FIND_ROOT_PATH}")
        list(PREPEND CMAKE_PREFIX_PATH "${__qt_find_package_host_qt_path}"
            ${_qt_additional_host_packages_prefix_paths})
        list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_HOST_PATH}"
            ${_qt_additional_host_packages_root_paths})
    endif()

    _qt_internal_save_find_package_context_for_debugging(Qt6${module})

    if(Qt6${module}_FOUND)
        # Tools packages don't usually provide a qt module, so there's no target.
        if(TARGET Qt6::${module})
            get_target_property(__qt_${module}_is_private Qt6::${module}
                _qt_is_private_module
            )
            if(__qt_${module}_is_private)
                _qt_internal_show_private_module_warning(${module})
            endif()
            unset(__qt_${module}_is_private)
        endif()
    else()
        find_package(Qt6${module}
            ${Qt6_FIND_VERSION}
            ${_Qt6_FIND_PARTS_QUIET}
            PATHS
                ${QT_BUILD_CMAKE_PREFIX_PATH}
                ${_qt_cmake_dir}
                ${_qt_additional_packages_prefix_paths}
                ${__qt_find_package_host_qt_path}
                ${_qt_additional_host_packages_prefix_paths}
            ${__qt_use_no_default_path_for_qt_packages}
        )
    endif()

    if(NOT "${__qt_find_package_host_qt_path}" STREQUAL "")
        set(CMAKE_PREFIX_PATH "${__qt_backup_cmake_prefix_path}")
        set(CMAKE_FIND_ROOT_PATH "${__qt_backup_cmake_find_root_path}")
        unset(__qt_backup_cmake_prefix_path)
        unset(__qt_backup_cmake_find_root_path)
        unset(__qt_find_package_host_qt_path)
    endif()

    if (NOT Qt6${module}_FOUND)
        set(_qt_expected_component_config_path
            "${_qt_cmake_dir}/Qt6${module}/Qt6${module}Config.cmake")
        get_filename_component(
            _qt_expected_component_dir_path "${_qt_expected_component_config_path}" DIRECTORY)

        set(_qt_component_not_found_msg
            "\nExpected Config file at \"${_qt_expected_component_config_path}\"")

        if(EXISTS "${_qt_expected_component_config_path}")
            string(APPEND _qt_component_not_found_msg " exists \n")
        else()
            string(APPEND _qt_component_not_found_msg " does NOT exist\n")
        endif()

        set(_qt_candidate_component_dir_path "${Qt6${module}_DIR}")

        if(_qt_candidate_component_dir_path AND
            NOT _qt_expected_component_dir_path STREQUAL _qt_candidate_component_dir_path)
            string(APPEND _qt_component_not_found_msg
               "\nQt6${module}_DIR was computed by CMake or specified on the "
               "command line by the user: \"${_qt_candidate_component_dir_path}\" "
               "\nThe expected and computed paths are different, which might be the reason for "
               "the package not to be found.")
        endif()

        if(Qt6_FIND_REQUIRED_${module})
            set(Qt6_FOUND False)
            set(_Qt_NOTFOUND_MESSAGE
                "${_Qt_NOTFOUND_MESSAGE}Failed to find required Qt component \"${module}\". ${_qt_component_not_found_msg}")
            set(_qt_full_component_name "Qt6${module}")
            _qt_internal_suggest_dependency_debugging(${_qt_full_component_name}
                _qt_full_component_name _Qt_NOTFOUND_MESSAGE)
            unset(_qt_full_component_name)
            break()
        elseif(NOT Qt6_FIND_QUIETLY)
            message(WARNING
                "Failed to find optional Qt component \"${module}\". ${_qt_component_not_found_msg}")
        endif()

        unset(_qt_expected_component_config_path)
        unset(_qt_expected_component_dir_path)
        unset(_qt_candidate_component_dir_path)
        unset(_qt_component_not_found_msg)
    endif()
endforeach()

if(Qt6_FIND_COMPONENTS AND _Qt_NOTFOUND_MESSAGE)
    set(Qt6_NOT_FOUND_MESSAGE "${_Qt_NOTFOUND_MESSAGE}")
    unset(_Qt_NOTFOUND_MESSAGE)
endif()

if(Qt6_FOUND
    AND COMMAND _qt_internal_override_example_install_dir_to_dot
    AND NOT _qt_internal_example_dir_set_to_dot)
    _qt_internal_override_example_install_dir_to_dot()
endif()

__qt_internal_defer_promote_targets_in_dir_scope_to_global()
if(CMAKE_VERSION VERSION_LESS 3.21)
    __qt_internal_check_link_order_matters()
    __qt_internal_check_cmp0099_available()
endif()
