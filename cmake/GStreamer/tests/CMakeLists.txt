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
    test_windows_sdk_version
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

if(QGC_ENABLE_GST_VIDEOSTREAMING AND NOT ANDROID AND NOT IOS)
    add_test(
        NAME FindModule.test_install_component
        COMMAND
            "${CMAKE_COMMAND}"
            "-DINSTALL_SCRIPT=${PROJECT_BINARY_DIR}/src/VideoManager/VideoReceiver/GStreamer/cmake_install.cmake"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/test_install_component.cmake"
    )
    set_tests_properties(
        FindModule.test_install_component PROPERTIES LABELS "Unit;FindModule" TIMEOUT 15
    )
endif()

set(_qgc_gst_argument_failure_cases
    "DownloadDelegatesRequired|download-delegates-required|qgc_resilient_download: missing values for: FILENAME"
    "DownloadResultRequired|download-result-required|gstreamer_resilient_download: RESULT_VAR is required"
    "DownloadUnknown|download-unknown|gstreamer_resilient_download: unknown arguments"
    "SecondaryArchivePair|secondary-archive-pair|FILENAME_SECONDARY and ARCHIVE2_OUT"
    "InstallUnknown|install-unknown|gstreamer_install_plugins: unknown arguments"
    "LayoutUnknown|layout-unknown|gstreamer_create_layout_target: unknown arguments"
    "PkgConfigUnknown|pkgconfig-unknown|gstreamer_apply_pkgconfig_env: unknown arguments"
    "PluginPolicyUnknown|plugin-policy-unknown|gstreamer_plugin_satisfy_sets: unknown arguments"
    "ProbeUnknown|probe-unknown|qgc_check_gst_header: unknown arguments"
)

foreach(qgc_failure_case IN LISTS _qgc_gst_argument_failure_cases)
    string(REPLACE "|" ";" _qgc_failure_parts "${qgc_failure_case}")
    list(GET _qgc_failure_parts 0 _qgc_test_name)
    list(GET _qgc_failure_parts 1 _qgc_test_case)
    list(GET _qgc_failure_parts 2 _qgc_expected_regex)
    add_test(
        NAME "FindModule.test_argument_validation.${_qgc_test_name}"
        COMMAND
            "${CMAKE_COMMAND}" "-DCMAKE_EXECUTABLE=${CMAKE_COMMAND}"
            "-DSCRIPT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/test_argument_validation.cmake"
            "-DTEST_CASE=${_qgc_test_case}" "-DEXPECTED_REGEX=${_qgc_expected_regex}" -P
            "${PROJECT_SOURCE_DIR}/cmake/tests/run_expected_failure_test.cmake"
    )
    set_tests_properties(
        "FindModule.test_argument_validation.${_qgc_test_name}"
        PROPERTIES LABELS "Unit;FindModule" TIMEOUT 15
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
