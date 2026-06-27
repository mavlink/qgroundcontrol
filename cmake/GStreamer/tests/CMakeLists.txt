include_guard(GLOBAL)

set(_qgc_findmod_tests
    test_json
    test_components
    test_download
    test_layout
    test_plugin_scan
    test_plugin_policy
    test_version_policy
    test_path_list
    test_pkgconfig_env
    test_download_helper
    test_install_filter
    test_windows_runtime_libproxy
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
