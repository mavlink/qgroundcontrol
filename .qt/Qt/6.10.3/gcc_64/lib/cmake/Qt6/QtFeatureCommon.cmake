# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_feature_normalize_name name out_var)
    # Normalize the feature name to something CMake can deal with.
    if(name MATCHES "c\\+\\+")
        string(REGEX REPLACE "[^a-zA-Z0-9_]" "x" name "${name}")
    else()
        string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" name "${name}")
    endif()
    set(${out_var} "${name}" PARENT_SCOPE)
endfunction()
