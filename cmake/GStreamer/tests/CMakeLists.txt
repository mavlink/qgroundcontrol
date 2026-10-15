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
    test_sdk_state
    test_install_libs_guard_windows
    test_windows_sdk_root
    test_platform
    test_checksum_authority
    test_ios_bundle_from_root
    test_sdk_acquire
    test_android_sdk_extract
    test_static_init
    test_ios_static_plugins
    test_ios_single_core
)

foreach(_t IN LISTS _qgc_findmod_tests)
    add_test(
        NAME FindModule.${_t}
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/${_t}.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties(FindModule.${_t} PROPERTIES LABELS "Unit;FindModule" TIMEOUT 15 FAIL_REGULAR_EXPRESSION
                                                     "ASSERT|FATAL_ERROR|CMake Error"
    )
endforeach()

if(QGC_ENABLE_GST_VIDEOSTREAMING
   AND NOT ANDROID
   AND NOT IOS
)
    add_test(
        NAME FindModule.test_install_component
        COMMAND
            "${CMAKE_COMMAND}"
            "-DINSTALL_SCRIPT=${PROJECT_BINARY_DIR}/src/VideoManager/VideoReceiver/GStreamer/cmake_install.cmake" -P
            "${CMAKE_CURRENT_SOURCE_DIR}/test_install_component.cmake"
    )
    set_tests_properties(FindModule.test_install_component PROPERTIES LABELS "Unit;FindModule" TIMEOUT 15)
endif()

# <script>|<test name suffix>|<TEST_CASE>|<expected diagnostic>. FindModule.<script>[.<suffix>] runs <script>.cmake,
# with TEST_CASE when given, and passes only when it fails with a diagnostic matching the expected regex, so unrelated
# errors cannot satisfy it.
set(_qgc_gst_failure_cases
    "test_argument_validation|AcquireExtractor|acquire-extractor|'_qgc_no_such_extractor' is not a command"
    "test_argument_validation|InstallUnknown|install-unknown|gstreamer_install_plugins: unknown arguments"
    "test_argument_validation|LayoutUnknown|layout-unknown|gstreamer_create_layout_target: unknown arguments"
    "test_argument_validation|PkgConfigUnknown|pkgconfig-unknown|gstreamer_apply_pkgconfig_env: unknown arguments"
    "test_argument_validation|ProbeUnknown|probe-unknown|qgc_check_gst_header: unknown arguments"
    "test_android_abi_info|||Unsupported Android ABI: mips"
    "test_checksum_authority|SidecarOnlyRequired|sidecar-only-required|cannot satisfy GStreamer_REQUIRE_CHECKSUM=ON"
    "test_components_malformed|||GSTREAMER_COMPONENT_REGISTRY: malformed entry 'OnlyThree:fields:here'"
    "test_plugin_policy_errors|||gstreamer_plugins_for: build-config.json not loaded"
    "test_plugin_policy_conflict|||'foo' is in both GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS and GSTREAMER_RUNTIME_REQUIRED"
    "test_install_libs_guard|||gstreamer_install_libs: refusing to copy from system/shared prefix"
    "test_download_win_version_guard|||Windows GStreamer SDK requires version 1.28 or later"
)
foreach(_qgc_failure_case IN LISTS _qgc_gst_failure_cases)
    string(REPLACE "|" ";" _qgc_failure_parts "${_qgc_failure_case}")
    list(GET _qgc_failure_parts 0 _qgc_script)
    list(GET _qgc_failure_parts 1 _qgc_test_suffix)
    list(GET _qgc_failure_parts 2 _qgc_test_case)
    list(GET _qgc_failure_parts 3 _qgc_expected_regex)
    set(_qgc_test_name "FindModule.${_qgc_script}")
    set(_qgc_test_case_arg "")
    if(_qgc_test_suffix)
        string(APPEND _qgc_test_name ".${_qgc_test_suffix}")
        set(_qgc_test_case_arg "-DTEST_CASE=${_qgc_test_case}")
    endif()
    add_test(
        NAME "${_qgc_test_name}"
        COMMAND
            "${CMAKE_COMMAND}" "-DCMAKE_EXECUTABLE=${CMAKE_COMMAND}"
            "-DSCRIPT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${_qgc_script}.cmake" ${_qgc_test_case_arg}
            "-DEXPECTED_REGEX=${_qgc_expected_regex}" -P
            "${PROJECT_SOURCE_DIR}/cmake/tests/run_expected_failure_test.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties("${_qgc_test_name}" PROPERTIES LABELS "Unit;FindModule" TIMEOUT 15)
endforeach()

# The Orchestrator fixture generates a Linux-layout fake SDK and drives it through the host's
# pkg-config, so it only runs on Linux hosts.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    foreach(
        _qgc_scenario IN
        ITEMS reconfigure
              static-targets
              reuse-auto-root
              legacy-pkg-config-args
              version-override
              missing-mandatory
              missing-gl
              install
              install-macos
              android-mobile
    )
        add_test(
            NAME "FindModule.test_orchestrator.${_qgc_scenario}"
            COMMAND
                "${CMAKE_COMMAND}" "-DQGC_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/test_orchestrator/${_qgc_scenario}"
                "-DSCENARIO=${_qgc_scenario}" "-DGENERATOR=${CMAKE_GENERATOR}" "-DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/test_orchestrator.cmake"
        )
        set_tests_properties("FindModule.test_orchestrator.${_qgc_scenario}"
                             PROPERTIES LABELS "Unit;FindModule" TIMEOUT 120 SKIP_REGULAR_EXPRESSION "QGC_TEST_SKIPPED"
        )
    endforeach()
endif()
