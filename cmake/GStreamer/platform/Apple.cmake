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
        message(
            FATAL_ERROR
                "Could not find pkg-config.\n"
                "Install dependencies with: python3 tools/setup/install_dependencies --platform macos\n"
                "or install pkg-config manually (for example: brew install pkg-config)."
        )
    endif()
    set(${out_var}
        "${_qgc_pkg_config}"
        CACHE FILEPATH "pkg-config executable" FORCE
    )
    unset(_qgc_pkg_config CACHE)
endfunction()
