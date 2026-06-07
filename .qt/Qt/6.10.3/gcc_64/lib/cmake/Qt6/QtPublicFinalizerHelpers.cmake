# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Helper to check if the finalizer mode should be used.
# If true, use finalizer mode.
# If false, use regular mode (usage requirement propagation via associated Qt module)
# Arguments:
# DEFAULT_VALUE specifies the default value of the finalizer mode flag if it is not set.
function(__qt_internal_check_finalizer_mode target out_var finalizer)
    set(option_args "")
    set(single_args DEFAULT_VALUE)
    set(multi_args "")
    cmake_parse_arguments(arg "${option_args}" "${single_args}" "${multi_args}" ${ARGN})

    if(NOT DEFINED arg_DEFAULT_VALUE OR arg_DEFAULT_VALUE)
        set(arg_DEFAULT_VALUE TRUE)
    else()
        set(arg_DEFAULT_VALUE FALSE)
    endif()
    get_target_property(value ${target} _qt_${finalizer}_finalizer_mode)
    if("${value}" STREQUAL "value-NOTFOUND")
        __qt_internal_enable_finalizer_mode(${target} ${finalizer} "${arg_DEFAULT_VALUE}")
        set(value "${arg_DEFAULT_VALUE}")
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(__qt_internal_enable_finalizer_mode target finalizer enabled)
    if(enabled)
        set(enabled "TRUE")
    else()
        set(enabled "FALSE")
    endif()
    set_property(TARGET "${target}" PROPERTY _qt_${finalizer}_finalizer_mode "${enabled}")
endfunction()
