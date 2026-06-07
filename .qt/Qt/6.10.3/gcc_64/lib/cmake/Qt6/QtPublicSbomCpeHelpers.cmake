# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Computes a security CPE for a given set of attributes.
#
# When a part is not specified, a wildcard is added.
#
# References:
# https://spdx.github.io/spdx-spec/v2.3/external-repository-identifiers/#f22-cpe23type
# https://nvlpubs.nist.gov/nistpubs/Legacy/IR/nistir7695.pdf
# https://nvd.nist.gov/products/cpe
#
# Each attribute means:
# 1. part
# 2. vendor
# 3. product
# 4. version
# 5. update
# 6. edition
# 7. language
# 8. sw_edition
# 9. target_sw
# 10. target_hw
# 11. other
function(_qt_internal_sbom_compute_security_cpe out_cpe)
    set(opt_args "")
    set(single_args
        PART
        VENDOR
        PRODUCT
        VERSION
        UPDATE
        EDITION
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(cpe_template "cpe:2.3:PART:VENDOR:PRODUCT:VERSION:UPDATE:EDITION:*:*:*:*:*")

    set(cpe "${cpe_template}")
    foreach(attribute_name IN LISTS single_args)
        if(arg_${attribute_name})
            set(${attribute_name}_value "${arg_${attribute_name}}")
        else()
            if(attribute_name STREQUAL "PART")
                set(${attribute_name}_value "a")
            else()
                set(${attribute_name}_value "*")
            endif()
        endif()
        string(REPLACE "${attribute_name}" "${${attribute_name}_value}" cpe "${cpe}")
    endforeach()

    set(${out_cpe} "${cpe}" PARENT_SCOPE)
endfunction()

# Computes the default security CPE for a given qt repository.
function(_qt_internal_sbom_get_cpe_qt_repo out_var)
    _qt_internal_sbom_get_root_project_name_lower_case(repo_project_name_lowercase)
    _qt_internal_sbom_compute_security_cpe(repo_cpe
        VENDOR "qt"
        PRODUCT "${repo_project_name_lowercase}"
        VERSION "${QT_REPO_MODULE_VERSION}"
    )
    set(${out_var} "${repo_cpe}" PARENT_SCOPE)
endfunction()

# Computes the default security CPE for the Qt framework.
function(_qt_internal_sbom_get_cpe_qt out_var)
    _qt_internal_sbom_compute_security_cpe(qt_cpe
        VENDOR "qt"
        PRODUCT "qt"
        VERSION "${QT_REPO_MODULE_VERSION}"
    )
    set(${out_var} "${qt_cpe}" PARENT_SCOPE)
endfunction()

# Computes the list of security CPEs for Qt, including both the repo-specific one and generic one.
function(_qt_internal_sbom_compute_security_cpe_for_qt out_cpe_list)
    set(cpe_list "")

    _qt_internal_sbom_get_cpe_qt(qt_cpe)
    list(APPEND cpe_list "${qt_cpe}")

    _qt_internal_sbom_get_cpe_qt_repo(repo_cpe)
    list(APPEND cpe_list "${repo_cpe}")

    set(${out_cpe_list} "${cpe_list}" PARENT_SCOPE)
endfunction()
