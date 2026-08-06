# Shared Apple (macOS + iOS) GStreamer helpers — invoked by Orchestrator.cmake.

function(_qgc_find_apple_pkg_config OUT_VAR)
    if(DEFINED CACHE{${OUT_VAR}} AND EXISTS "$CACHE{${OUT_VAR}}")
        return()
    endif()
    find_program(_qgc_pkg_config
        NAMES pkg-config pkgconf
        PATHS /opt/homebrew/bin /usr/local/bin
        NO_DEFAULT_PATH
    )
    if(NOT _qgc_pkg_config)
        find_program(_qgc_pkg_config NAMES pkg-config pkgconf)
    endif()
    if(NOT _qgc_pkg_config)
        message(FATAL_ERROR
            "Could not find pkg-config.\n"
            "Install dependencies with: python3 tools/setup/install_dependencies.py --platform macos\n"
            "or install pkg-config manually (for example: brew install pkg-config).")
    endif()
    set(${OUT_VAR} "${_qgc_pkg_config}" CACHE FILEPATH "pkg-config executable" FORCE)
    unset(_qgc_pkg_config CACHE)
endfunction()

# Validates a pkgutil-expanded GStreamer package directory. Used by the
# CPM-downloaded SDK paths in macOS and iOS discovery macros.
function(_qgc_validate_expanded_pkg EXPANDED_DIR LABEL)
    file(GLOB _payloads "${EXPANDED_DIR}/*.pkg/Payload")
    if(NOT _payloads)
        file(REMOVE_RECURSE "${EXPANDED_DIR}")
        message(FATAL_ERROR
            "pkgutil expanded GStreamer ${LABEL} package but no payloads were found in ${EXPANDED_DIR}")
    endif()
endfunction()

# Expand a .pkg with pkgutil --expand-full into EXPANDED_DIR, FATAL on failure,
# then validate payloads. Shared by macOS (runtime+devel) and iOS discovery.
function(_qgc_pkgutil_expand_and_validate PKG EXPANDED_DIR LABEL)
    message(STATUS "Expanding GStreamer ${LABEL} package...")
    execute_process(
        COMMAND pkgutil --expand-full "${PKG}" "${EXPANDED_DIR}"
        RESULT_VARIABLE _pkgutil_rc
    )
    if(NOT _pkgutil_rc EQUAL 0)
        file(REMOVE_RECURSE "${EXPANDED_DIR}")
        message(FATAL_ERROR "pkgutil failed to expand GStreamer ${LABEL} .pkg (exit code: ${_pkgutil_rc})")
    endif()
    _qgc_validate_expanded_pkg("${EXPANDED_DIR}" "${LABEL}")
endfunction()
