# Shared Apple (macOS + iOS) GStreamer helpers — invoked by Orchestrator.cmake.

include_guard(GLOBAL)

# Sets the cache entry <out_var> to a pkg-config executable, preferring Homebrew's; an existing entry is kept.
function(_qgc_gst_find_apple_pkg_config out_var)
    if(DEFINED CACHE{${out_var}} AND EXISTS "$CACHE{${out_var}}")
        return()
    endif()
    find_program(
        _qgc_pkg_config
        NAMES pkg-config pkgconf
        PATHS /opt/homebrew/bin /usr/local/bin
        NO_DEFAULT_PATH
    )
    if(NOT _qgc_pkg_config)
        find_program(_qgc_pkg_config NAMES pkg-config pkgconf)
    endif()
    if(NOT _qgc_pkg_config)
        message(FATAL_ERROR "Could not find pkg-config.\n"
                            "Install dependencies with: python3 tools/setup/install_dependencies.py --platform macos\n"
                            "or install pkg-config manually (for example: brew install pkg-config)."
        )
    endif()
    set(${out_var}
        "${_qgc_pkg_config}"
        CACHE FILEPATH "pkg-config executable" FORCE
    )
    unset(_qgc_pkg_config CACHE)
endfunction()

# Expands <pkg> with pkgutil --expand-full into <expanded_dir> and fails configure unless that yields package payloads.
# Used by the macOS (runtime+devel) and iOS SDK extractors; qgc_gst_acquire_sdk clears their staging directory, which
# holds <expanded_dir>, before every attempt.
function(_qgc_gst_pkgutil_expand_and_validate pkg expanded_dir label)
    message(STATUS "Expanding GStreamer ${label} package...")
    execute_process(COMMAND pkgutil --expand-full "${pkg}" "${expanded_dir}" RESULT_VARIABLE _pkgutil_rc)
    if(NOT _pkgutil_rc EQUAL 0)
        message(FATAL_ERROR "pkgutil failed to expand GStreamer ${label} .pkg (exit code: ${_pkgutil_rc})")
    endif()
    file(GLOB _payloads "${expanded_dir}/*.pkg/Payload")
    if(NOT _payloads)
        message(FATAL_ERROR "pkgutil expanded GStreamer ${label} package but no payloads were found in ${expanded_dir}")
    endif()
endfunction()
