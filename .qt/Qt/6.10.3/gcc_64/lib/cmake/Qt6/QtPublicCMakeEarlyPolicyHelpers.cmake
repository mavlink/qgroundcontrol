# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Save the current value of the CMP0156 policy in a propert of the current directory scope.
function(__qt_internal_save_directory_scope_policy_cmp0156)
    if(NOT POLICY CMP0156)
        return()
    endif()

    # Exit early if we already saved the policy value for this directory scope.
    get_property(policy_value_set DIRECTORY PROPERTY _qt_internal_policy_cmp0156_value_set)
    if(policy_value_set)
        return()
    endif()

    cmake_policy(GET CMP0156 policy_value)
    set_property(DIRECTORY PROPERTY _qt_internal_policy_cmp0156_value "${policy_value}")
    set_property(DIRECTORY PROPERTY _qt_internal_policy_cmp0156_value_set "TRUE")
endfunction()

function(__qt_internal_get_directory_scope_policy_cmp0156 out_var)
    get_property(policy_value DIRECTORY PROPERTY _qt_internal_policy_cmp0156_value)
    set(${out_var} "${policy_value}" PARENT_SCOPE)
endfunction()
