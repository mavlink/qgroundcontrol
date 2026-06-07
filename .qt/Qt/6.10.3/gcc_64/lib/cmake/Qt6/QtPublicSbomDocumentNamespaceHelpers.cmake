# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Computes the SPDX document namespace field.
# See https://spdx.github.io/spdx-spec/v2.3/document-creation-information/#65-spdx-document-namespace-field
# The document namespace is used in SPDX external references and dependency relationships.
function(_qt_internal_sbom_compute_project_namespace out_var)
    set(opt_args "")
    set(single_args
        SUPPLIER_URL
        PROJECT_NAME
        VERSION_SUFFIX
        DOCUMENT_NAMESPACE_INFIX
        DOCUMENT_NAMESPACE_SUFFIX
        DOCUMENT_NAMESPACE_URL_PREFIX
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PROJECT_NAME)
        message(FATAL_ERROR "PROJECT_NAME must be set")
    endif()

    if(NOT arg_SUPPLIER_URL)
        message(FATAL_ERROR "SUPPLIER_URL must be set")
    endif()

    string(TOLOWER "${arg_PROJECT_NAME}" project_name_lowercase)

    set(version_suffix "")

    if(arg_VERSION_SUFFIX)
        set(version_suffix "-${arg_VERSION_SUFFIX}")
    else()
        _qt_internal_sbom_get_git_version_vars()
        if(QT_SBOM_GIT_VERSION)
            set(version_suffix "-${QT_SBOM_GIT_VERSION}")
        endif()
    endif()

    set(namespace "${project_name_lowercase}${version_suffix}")

    if(arg_DOCUMENT_NAMESPACE_INFIX)
        string(APPEND namespace "${arg_DOCUMENT_NAMESPACE_INFIX}")
    endif()

    if(arg_DOCUMENT_NAMESPACE_SUFFIX)
        string(APPEND namespace "${arg_DOCUMENT_NAMESPACE_SUFFIX}")
    endif()

    if(arg_DOCUMENT_NAMESPACE_URL_PREFIX)
        set(url_prefix "${arg_DOCUMENT_NAMESPACE_URL_PREFIX}")
    else()
        set(url_prefix "${arg_SUPPLIER_URL}/spdxdocs")
    endif()

    set(repo_spdx_namespace "${url_prefix}/${namespace}")

    set(${out_var} "${repo_spdx_namespace}" PARENT_SCOPE)
endfunction()

# A document namespace is recommended to be either a URI + v4 random UUID or a URI + v5 UUID
# generated from a sha1 checksum.
# It needs to be unique per document.
# Having randomness is bad for build reproducibility, so the v4 UUID is not a good idea.
#
# Collecting enough unique content as part of the build for a checksum is difficult
# without outside input, and because the final document contents is only available at install time.
#
# We currently create a fake URI (that is not hosted on a web service) and a combination of the
# following info:
# - project name
# - project version
# - document namespace infix, which consists of:
#   - platform and arch info (host + target)
#   - a v5 UUID based on various inputs
#   - extra content provided as input
# - document namespace suffix (as a last resort)
#
# The document namespace infix should make the namespace unique enough, so that different
# builds don't usually map to the same value, and thus is conformant to the spec.
function(_qt_internal_sbom_compute_uniqueish_document_namespace_infix)
    set(opt_args "")
    set(single_args
        UUID_EXTRA_CONTENT
        OUT_VAR_INFIX
        OUT_VAR_UUID_INFIX
        OUT_VAR_UUID_INFIX_MERGED
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR_INFIX AND NOT arg_OUT_VAR_UUID_INFIX AND NOT arg_OUT_VAR_UUID_INFIX_MERGED)
        message(FATAL_ERROR "One of OUT_VAR_INFIX, OUT_VAR_UUID_INFIX or "
            "OUT_VAR_UUID_INFIX_MERGED must be set")
    endif()

    if(QT_SBOM_NO_UNIQUE_NAMESPACE_INFIX)
        set(${arg_OUT_VAR_INFIX} "" PARENT_SCOPE)

        if(arg_OUT_VAR_UUID_INFIX)
            set(${arg_OUT_VAR_UUID_INFIX} "" PARENT_SCOPE)
        endif()

        if(arg_OUT_VAR_UUID_INFIX_MERGED)
            set(${arg_OUT_VAR_UUID_INFIX_MERGED} "" PARENT_SCOPE)
        endif()

        return()
    endif()

    # Collect the various pieces of information to build the unique-ish infix.
    set(main_value "")

    _qt_internal_sbom_get_host_platform_name(host_platform_name)
    if(host_platform_name)
        string(APPEND main_value "host-${host_platform_name}")
    endif()

    _qt_internal_sbom_get_host_platform_architecture(host_arch)
    if(host_arch)
        string(APPEND main_value "-${host_arch}")
    endif()

    _qt_internal_sbom_get_target_platform_friendly_name(target_platform_name)
    if(target_platform_name)
        string(APPEND main_value "-target-${target_platform_name}")
    endif()

    _qt_internal_sbom_get_target_platform_architecture(target_arch)
    if(target_arch)
        string(APPEND main_value "-${target_arch}")
    endif()


    # Collect the pieces for the infix uuid part.
    set(uuid_content "<main_value>:${main_value}\n")

    _qt_internal_sbom_get_build_tools_info_for_namespace_infix_uuid(tools_info)
    if(tools_info)
        string(APPEND uuid_content "<build_tools_info>:${tools_info}\n")
    endif()

    if(arg_UUID_EXTRA_CONTENT)
        string(APPEND uuid_content "<extra_content>:\n${arg_UUID_EXTRA_CONTENT}\n")
    endif()

    if(QT_SBOM_NAMESPACE_INFIX_UUID_EXTRA_CONTENT)
        string(APPEND uuid_content
            "<extra_content_var>:${QT_SBOM_NAMESPACE_INFIX_UUID_EXTRA_CONTENT}\n")
    endif()

    _qt_internal_sbom_compute_document_namespace_infix_uuid(
        UUID_CONTENT "${uuid_content}"
        OUT_VAR_UUID uuid_value
    )

    if(arg_OUT_VAR_INFIX)
        set(${arg_OUT_VAR_INFIX} "${main_value}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_UUID_INFIX)
        set(${arg_OUT_VAR_UUID_INFIX} "${uuid_value}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_UUID_INFIX_MERGED)
        set(${arg_OUT_VAR_UUID_INFIX_MERGED} "${main_value}-${uuid_value}" PARENT_SCOPE)
    endif()
endfunction()

# Computes the uuid part of a SPDX document namespace, given the inputs.
# UUID_CONTENT - should be given enough unique content to ensure the uniqueness of generated the
# uuid based on the content.
#
# Allow various overrides like:
# - override of the full uuid content via QT_SBOM_FORCE_DOCUMENT_NAMESPACE_INFIX_UUID_CONTENT
# - allow using a random value via QT_SBOM_FORCE_RANDOM_DOCUMENT_NAMESPACE_INFIX_UUID_CONTENT
# - allow setting a specific uuid via QT_SBOM_FORCE_DOCUMENT_NAMESPACE_INFIX_UUID
# - fake deterministic uuid (only useful for development purposes of this code)
function(_qt_internal_sbom_compute_document_namespace_infix_uuid)
    set(opt_args "")
    set(single_args
        UUID_CONTENT
        OUT_VAR_UUID
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR_UUID)
        message(FATAL_ERROR "OUT_VAR_UUID must be set")
    endif()

    set(content "${arg_UUID_CONTENT}")

    _qt_internal_sbom_get_document_namespace_uuid_namespace(uuid_namespace)

    # Allow various overrides.
    if(QT_SBOM_FAKE_DETERMINISTIC_BUILD
            # This is to allow developers test a fake build, without a fake uuid
            AND NOT QT_SBOM_NO_FAKE_DETERMINISTIC_BUILD_DOCUMENT_NAMESPACE_INFIX_UUID
        )
        set(uuid_content "<fake_deterministic_build>")
    elseif(QT_SBOM_FORCE_DOCUMENT_NAMESPACE_INFIX_UUID_CONTENT)
        set(uuid_content "${QT_SBOM_FORCE_DOCUMENT_NAMESPACE_INFIX_UUID_CONTENT}")
    elseif(QT_SBOM_FORCE_RANDOM_DOCUMENT_NAMESPACE_INFIX_UUID_CONTENT)
        string(RANDOM LENGTH 256 uuid_content)
    else()
        set(uuid_content "${content}")
    endif()

    # Also allow direct override of uuid.
    if(QT_SBOM_FORCE_DOCUMENT_NAMESPACE_INFIX_UUID)
        set(namespace_infix_uuid "${QT_SBOM_FORCE_DOCUMENT_NAMESPACE_INFIX_UUID}")
    else()
        string(UUID namespace_infix_uuid
            NAMESPACE "${uuid_namespace}" NAME "${uuid_content}" TYPE SHA1)
    endif()

    set(${arg_OUT_VAR_UUID} "${namespace_infix_uuid}" PARENT_SCOPE)
endfunction()

# A v4 uuid to be used as a namespace value for generating v5 uuids.
function(_qt_internal_sbom_get_document_namespace_uuid_namespace out_var)
    # This is a randomly generated uuid v4 value. To be used for all eternity. Until we change the
    # implementation of the function.
    set(uuid_namespace "c024642f-9853-45b2-9bfd-ab3f061a05bb")
    set(${out_var} "${uuid_namespace}" PARENT_SCOPE)
endfunction()

# Collects extra uuid content for generating a more unique document namespace uuid for qt repos.
function(_qt_internal_sbom_compute_qt_uniqueish_document_namespace_infix)
    set(opt_args "")
    set(single_args
        OUT_VAR_INFIX
        OUT_VAR_UUID_INFIX
        OUT_VAR_UUID_INFIX_MERGED
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR_INFIX AND NOT arg_OUT_VAR_UUID_INFIX AND NOT arg_OUT_VAR_UUID_INFIX_MERGED)
        message(FATAL_ERROR "One of OUT_VAR_INFIX, OUT_VAR_UUID_INFIX or "
            "OUT_VAR_UUID_INFIX_MERGED must be set")
    endif()

    if(QT_SBOM_NO_UNIQUE_QT_NAMESPACE_INFIX)
        set(${arg_OUT_VAR_INFIX} "" PARENT_SCOPE)

        if(arg_OUT_VAR_UUID_INFIX)
            set(${arg_OUT_VAR_UUID_INFIX} "" PARENT_SCOPE)
        endif()

        if(arg_OUT_VAR_UUID_INFIX_MERGED)
            set(${arg_OUT_VAR_UUID_INFIX_MERGED} "" PARENT_SCOPE)
        endif()

        return()
    endif()

    set(uuid_extra_content "")

    if(APPLE AND (CMAKE_OSX_ARCHITECTURES MATCHES ";"))
        string(CONCAT building_for
            "${QT_QMAKE_TARGET_MKSPEC} (${CMAKE_OSX_ARCHITECTURES}), ${TEST_architecture_arch} "
            "features: ${subarch_summary})")
    else()
        string(CONCAT building_for
            "${QT_QMAKE_TARGET_MKSPEC} (${TEST_architecture_arch}, "
                "CPU features: ${subarch_summary})")
    endif()

    string(APPEND uuid_extra_content "<building_for>:${building_for}\n")

    _qt_internal_get_configure_line(configure_line)
    if(configure_line)
        string(APPEND uuid_extra_content "<configure_line>:${configure_line}\n")
    endif()

    _qt_internal_sbom_compute_uniqueish_document_namespace_infix(
        UUID_EXTRA_CONTENT "${uuid_extra_content}"
        OUT_VAR_INFIX infix
        OUT_VAR_UUID_INFIX uuid_infix
        OUT_VAR_UUID_INFIX_MERGED uuid_infix_merged
    )

    if(arg_OUT_VAR_INFIX)
        set(${arg_OUT_VAR_INFIX} "${infix}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_UUID_INFIX)
        set(${arg_OUT_VAR_UUID_INFIX} "${uuid_infix}" PARENT_SCOPE)
    endif()

    if(arg_OUT_VAR_UUID_INFIX_MERGED)
        set(${arg_OUT_VAR_UUID_INFIX_MERGED} "${uuid_infix_merged}" PARENT_SCOPE)
    endif()
endfunction()

# Computes a short unique-ish sha suffix to embed into all Qt package SPDX ids, to ensure that each
# package spdx id is distinct enough between Qt builds.
#
# The use case is referencing multiple versions of the the same target, within a project that
# mixes multiple Qt versions, like Qt for MCU.
#
# E.g. record that MyLib depends on Qt Core 6.8.3 from qtbase spdx doc1, and that MyLib2 depends
# on Qt Core 6.9.1 from qtbase spdx doc2, within the generated project doc that contains MyLib and
# MyLib2.
#
# Default hash length chosen to be 12, to have a probability of 0.001 collisions when there are
# 750000 releases of the same Qt repo, according to
# https://en.wikipedia.org/wiki/Birthday_problem#Probability_table
function(_qt_internal_sbom_compute_uniqueish_spdx_id_suffix)
    set(opt_args "")
    set(single_args
        SPDX_NAMESPACE
        OUT_VAR_UNIQUE_SUFFIX
        HASH_LENGTH
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(QT_SBOM_NO_AUTO_SPDX_SUFFIX)
        set(${arg_OUT_VAR_UNIQUE_SUFFIX} "" PARENT_SCOPE)
        return()
    endif()

    if(NOT arg_OUT_VAR_UNIQUE_SUFFIX)
        message(FATAL_ERROR "OUT_VAR_UNIQUE_SUFFIX must be set")
    endif()

    if(NOT arg_SPDX_NAMESPACE)
        message(FATAL_ERROR "SPDX_NAMESPACE must be set")
    endif()

    if(QT_SBOM_FAKE_DETERMINISTIC_BUILD
            # This is to allow developers test a fake build, without a fake uuid
            AND NOT QT_SBOM_NO_FAKE_DETERMINISTIC_BUILD_SPDX_IDX_SUFFIX
        )
        set(unique_suffix "fake-id-suffix")
    else()
        string(SHA1 namespace_sha "${arg_SPDX_NAMESPACE}")

        if(arg_HASH_LENGTH)
            set(sha_suffix_length "${arg_HASH_LENGTH}")
        else()
            set(sha_suffix_length "12")
        endif()

        string(SUBSTRING "${namespace_sha}" 0 "${sha_suffix_length}" unique_suffix)
    endif()

    set(${arg_OUT_VAR_UNIQUE_SUFFIX} "${unique_suffix}" PARENT_SCOPE)
endfunction()

# Retrieves the current project's spdx id suffix.
function(_qt_internal_sbom_get_spdx_id_unique_suffix out_var)
    get_cmake_property(unique_suffix _qt_internal_sbom_repo_spdx_id_unique_suffix)
    if(NOT unique_suffix)
        set(unique_suffix "")
    endif()

    set(${out_var} "${unique_suffix}" PARENT_SCOPE)
endfunction()

# Returns a lower case host platform name for sbom document namespace purposes.
function(_qt_internal_sbom_get_host_platform_name out_var)
    string(TOLOWER "${CMAKE_HOST_SYSTEM_NAME}" main_value)
    if(NOT main_value)
        set(main_value "unknown-platform")
    endif()

    set(${out_var} "${main_value}" PARENT_SCOPE)
endfunction()

# Returns a lower case target platform name for sbom document namespace purposes.
function(_qt_internal_sbom_get_target_platform_friendly_name out_var)
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" lower_system_name)
    set(friendly_name "${lower_system_name}")

    if(NOT friendly_name)
        set(friendly_name "unknown-platform")
    endif()

    if(MSVC)
        string(APPEND friendly_name "-msvc")
    endif()

    if(MINGW)
        string(APPEND friendly_name "-mingw")
    endif()

    if(CYGWIN)
        string(APPEND friendly_name "-cygwin")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(friendly_name "linux")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "HPUX")
        set(friendly_name "hpux")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Android")
        set(friendly_name "android")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Integrity")
        set(friendly_name "integrity")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "VxWorks")
        set(friendly_name "vxworks")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
        set(friendly_name "qnx")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
        set(friendly_name "openbsd")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(friendly_name "freebsd")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
        set(friendly_name "netbsd")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" OR EMSCRIPTEN)
        set(friendly_name "wasm")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
        set(friendly_name "sunos")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "GNU")
        set(friendly_name "hurd")
    endif()

    if(CMAKE_CXX_FLAGS MATCHES "-D__WEBOS__")
        set(friendly_name "webos")
    endif()

    if(APPLE)
        if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
            set(friendly_name "ios")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "tvOS")
            set(friendly_name "tvos")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "watchOS")
            set(friendly_name "watchos")
        elseif(CMAKE_SYSTEM_NAME STREQUAL "visionOS")
            set(friendly_name "visionos")
        else()
            set(friendly_name "macos")
        endif()
    endif()

    set(${out_var} "${friendly_name}" PARENT_SCOPE)
endfunction()

# Returns the host architecture for sbom document namespace purposes.
function(_qt_internal_sbom_get_host_platform_architecture out_var)
    set(main_value "${CMAKE_HOST_SYSTEM_PROCESSOR}")

    if(QT_SBOM_HOST_PLATFORM_ARCHITECTURE)
        set(main_value "${QT_SBOM_HOST_PLATFORM_ARCHITECTURE}")
    endif()

    string(TOLOWER "${main_value}" main_value)

    set(${out_var} "${main_value}" PARENT_SCOPE)
endfunction()

# Returns the target architecture for sbom document namespace purposes.
function(_qt_internal_sbom_get_target_platform_architecture out_var)
    set(main_value "")
    if(APPLE)
        set(main_value "${CMALE_OSX_ARCHITECTURES}")
        string(REPLACE ";" "_" main_value "${main_value}")
    endif()

    if(NOT main_value)
        set(main_value "${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    if(QT_SBOM_TARGET_PLATFORM_ARCHITECTURE)
        set(main_value "${QT_SBOM_TARGET_PLATFORM_ARCHITECTURE}")
    endif()

    string(TOLOWER "${main_value}" main_value)

    set(${out_var} "${main_value}" PARENT_SCOPE)
endfunction()

# Returns various build tool information ofr document namespace purposes.
function(_qt_internal_sbom_get_build_tools_info_for_namespace_infix_uuid out_var)
    set(content "")

    string(APPEND content "<cmake_version>: ${CMAKE_VERSION}\n")
    string(APPEND content "<cmake_generator>: ${CMAKE_GENERATOR}\n")
    string(APPEND content "<compiler>: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\n")

    if(CMAKE_CXX_COMPILER_LINKER_ID)
        string(APPEND content "<linker>: ${CMAKE_CXX_COMPILER_LINKER_ID} "
            "${CMAKE_CXX_COMPILER_LINKER_VERSION} "
            "${CMAKE_CXX_COMPILER_LINKER_FRONTEND_VARIANT}\n")
    endif()

    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()
