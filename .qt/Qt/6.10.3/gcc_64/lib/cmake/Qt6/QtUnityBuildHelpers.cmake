# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Check whether no unity build is requested where it is disabled by default.
function(_qt_internal_validate_no_unity_build prefix)
    if(${prefix}_NO_UNITY_BUILD OR ${prefix}_NO_UNITY_BUILD_SOURCES)
        message(WARNING
            "Unity build is disabled by default for this target, and its sources. "
            "You may remove the NO_UNITY_BUILD and/or NO_UNITY_BUILD_SOURCES arguments.")
    endif()
endfunction()

function(qt_update_ignore_unity_build_sources target sources)
    if(sources)
        # We need to add the TARGET_DIRECTORY scope for targets that have qt_internal_extend_target
        # calls in different subdirectories, like in qtgraphs.
        set(scope_args)
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
            set(scope_args TARGET_DIRECTORY ${target})
        endif()
        set_source_files_properties(${sources} ${scope_args}
            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
    endif()
endfunction()
