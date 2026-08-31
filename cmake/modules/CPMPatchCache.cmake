include_guard(GLOBAL)

# Derive a deterministic CPM source-cache key from a Git origin and patch contents.
function(qgc_cpm_patch_cache_key output_variable)
    if(NOT "${output_variable}" MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "qgc_cpm_patch_cache_key: output variable name is required")
    endif()

    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "GIT_REPOSITORY;GIT_TAG" "PATCHES")
    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "qgc_cpm_patch_cache_key: missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_cpm_patch_cache_key: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

    foreach(required GIT_REPOSITORY GIT_TAG)
        if(NOT DEFINED ARG_${required} OR "${ARG_${required}}" STREQUAL "")
            message(FATAL_ERROR "qgc_cpm_patch_cache_key: ${required} is required")
        endif()
    endforeach()
    if(NOT ARG_PATCHES)
        message(FATAL_ERROR "qgc_cpm_patch_cache_key: at least one patch is required")
    endif()

    set(key_material "repository=${ARG_GIT_REPOSITORY}\ntag=${ARG_GIT_TAG}")
    foreach(patch IN LISTS ARG_PATCHES)
        get_filename_component(patch_path "${patch}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        if(NOT EXISTS "${patch_path}" OR IS_DIRECTORY "${patch_path}")
            message(FATAL_ERROR "qgc_cpm_patch_cache_key: patch does not exist: ${patch_path}")
        endif()

        if(NOT CMAKE_SCRIPT_MODE_FILE)
            set_property(
                DIRECTORY
                APPEND
                PROPERTY CMAKE_CONFIGURE_DEPENDS "${patch_path}"
            )
        endif()
        file(SHA256 "${patch_path}" patch_sha256)
        string(APPEND key_material "\npatch=${patch_sha256}")
    endforeach()

    string(SHA256 cache_key "${key_material}")
    set(${output_variable}
        "${cache_key}"
        PARENT_SCOPE
    )
endfunction()
