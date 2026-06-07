# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(__qt_internal_collect_plugin_targets_from_dependencies_v2 target out_var)
    if(CMAKE_VERSION VERSION_LESS 3.30)
        __qt_internal_collect_plugin_targets_from_dependencies("${target}" "${out_var}")
        set(${out_var} "${${out_var}}" PARENT_SCOPE)
        return()
    endif()
    set("${out_var}" "$<TARGET_PROPERTY:${target},QT_PLUGIN_TARGETS>" PARENT_SCOPE)
endfunction()

function(__qt_internal_collect_plugin_library_files_v2 target plugin_targets out_var)
    if(CMAKE_VERSION VERSION_LESS 3.30)
        __qt_internal_collect_plugin_library_files("${target}" "${plugin_targets}" "${out_var}")
        set(${out_var} "${${out_var}}" PARENT_SCOPE)
        return()
    endif()

    set(plugin_targets "$<GENEX_EVAL:${plugin_targets}>")

    # Convert the list of plugin targets to a list of plugin files
    set(pre_genex "$$<1:<TARGET_FILE:>")
    set(post_genex "$<ANGLE-R>")
    set(glue "${post_genex};${pre_genex}")
    set("${out_var}"
        "$<$<BOOL:${plugin_targets}>:$<GENEX_EVAL:${pre_genex}$<JOIN:${plugin_targets},${glue}>${post_genex}>>"
        PARENT_SCOPE
    )
endfunction()
