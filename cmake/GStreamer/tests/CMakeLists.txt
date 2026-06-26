# Standalone CTest harness for cmake/GStreamer/* — runs cmake -P scripts
# that exercise the focused submodules without configuring/building the
# project. Wired into the build via add_subdirectory() from the root
# CMakeLists.txt under if(QGC_BUILD_TESTING).
#
# Each test is a self-contained .cmake script that:
#   1. Adds cmake/GStreamer to CMAKE_MODULE_PATH
#   2. include()s the focused submodule under test
#   3. Invokes _assert.cmake helpers; aborts on first mismatch.

include_guard(GLOBAL)

set(_qgc_findmod_tests
    test_json
    test_components
    test_download
    test_layout
    test_plugin_scan
    test_plugin_policy
    test_path_list
    test_pkgconfig_env
    test_download_helper
    test_install_filter
    test_install_libs_guard_windows
    test_orchestrator_idempotent
)

foreach(_t IN LISTS _qgc_findmod_tests)
    add_test(
        NAME FindModule.${_t}
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/${_t}.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties(FindModule.${_t} PROPERTIES
        LABELS "Unit;FindModule"
        TIMEOUT 15
        FAIL_REGULAR_EXPRESSION "ASSERT|FATAL_ERROR|CMake Error"
    )
endforeach()

# Negative tests: each script is expected to abort (FATAL_ERROR / non-zero exit).
# WILL_FAIL inverts the verdict, so a clean exit here is the real failure. No
# FAIL_REGULAR_EXPRESSION — the FATAL_ERROR output is the expected success path.
set(_qgc_findmod_willfail_tests
    test_components_malformed
    test_plugin_policy_errors
    test_plugin_policy_conflict
    test_install_libs_guard
    test_download_win_version_guard
)

foreach(_t IN LISTS _qgc_findmod_willfail_tests)
    add_test(
        NAME FindModule.${_t}
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/${_t}.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties(FindModule.${_t} PROPERTIES
        LABELS "Unit;FindModule"
        TIMEOUT 15
        WILL_FAIL TRUE
    )
endforeach()
