include_guard(GLOBAL)

include("${PROJECT_SOURCE_DIR}/cmake/tests/ExpectedFailureTest.cmake")

# ctest -L CMake runs these together with the shared-module tests of cmake/tests.
set(_qgc_gst_test_labels Unit CMake GStreamer)

set(_qgc_gst_tests
    test_components
    test_download
    test_layout
    test_plugin_scan
    test_plugin_policy
    test_version_policy
    test_path_list
    test_pkgconfig_env
    test_install_filter
    test_sdk_state
    test_install_libs_guard_windows
    test_windows_sdk_root
    test_windows_installer_guard
    test_platform
    test_checksum_authority
    test_ios_bundle_from_root
    test_ios_xcframework_slice
    test_sdk_acquire
    test_android_sdk_extract
    test_android_pkg_config
    test_static_init
    test_ios_static_plugins
    test_ios_single_core
    test_ios_sdk_version
    test_notice
)

foreach(_t IN LISTS _qgc_gst_tests)
    add_test(
        NAME GStreamer.${_t}
        COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_SOURCE_DIR}/${_t}.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties(GStreamer.${_t} PROPERTIES LABELS "${_qgc_gst_test_labels}" TIMEOUT 15 FAIL_REGULAR_EXPRESSION
                                                    "ASSERT|FATAL_ERROR|CMake Error"
    )
endforeach()
# The stand-in installer is a shell script, which a Windows host cannot run.
set_tests_properties(GStreamer.test_windows_installer_guard PROPERTIES SKIP_REGULAR_EXPRESSION "QGC_TEST_SKIPPED")

if(QGC_ENABLE_GST_VIDEOSTREAMING
   AND NOT ANDROID
   AND NOT IOS
)
    add_test(
        NAME GStreamer.test_install_component
        COMMAND
            "${CMAKE_COMMAND}"
            "-DINSTALL_SCRIPT=${PROJECT_BINARY_DIR}/src/VideoManager/VideoReceiver/GStreamer/cmake_install.cmake" -P
            "${CMAKE_CURRENT_SOURCE_DIR}/test_install_component.cmake"
    )
    set_tests_properties(GStreamer.test_install_component PROPERTIES LABELS "${_qgc_gst_test_labels}" TIMEOUT 15)
endif()

# <script>|<test name suffix>|<TEST_CASE>|<expected diagnostic>. GStreamer.<script>[.<suffix>] runs <script>.cmake,
# with TEST_CASE when given, and must fail with that diagnostic (qgc_add_expected_failure_test).
set(_qgc_gst_failure_cases
    "test_argument_validation|AcquireExtractor|acquire-extractor|'_qgc_no_such_extractor' is not a command"
    "test_argument_validation|LayoutUnknown|layout-unknown|gstreamer_create_layout_target: unknown arguments"
    "test_argument_validation|PkgConfigUnknown|pkgconfig-unknown|gstreamer_apply_pkgconfig_env: unknown arguments"
    "test_argument_validation|ProbeUnknown|probe-unknown|qgc_check_gst_header: unknown arguments"
    "test_android_abi_info|||Unsupported Android ABI: mips"
    "test_checksum_authority|SidecarOnlyRequired|sidecar-only-required|cannot satisfy GStreamer_REQUIRE_CHECKSUM=ON"
    "test_components|UnknownComponent|unknown|unknown components Bogus"
    "test_plugin_policy_errors|||qgc_config_read: build-config.json is not loaded"
    "test_install_libs_guard|||gstreamer_install_libs: refusing to copy from system/shared prefix"
    "test_ios_xcframework_slice|NoMatchingArch|no-matching-arch|no iphoneos slice for architectures 'x86_64'"
    "test_download_win_version_guard|||Windows GStreamer SDK requires version 1.28 or later"
    "test_windows_installer_guard|Unpinned|unpinned|refusing to silently run the downloaded Windows installer"
    "test_ios_sdk_version|BelowFloor|below-floor|older than the required 1.28 floor"
)
foreach(_qgc_failure_case IN LISTS _qgc_gst_failure_cases)
    string(REPLACE "|" ";" _qgc_failure_parts "${_qgc_failure_case}")
    list(GET _qgc_failure_parts 0 _qgc_script)
    list(GET _qgc_failure_parts 1 _qgc_test_suffix)
    list(GET _qgc_failure_parts 2 _qgc_test_case)
    list(GET _qgc_failure_parts 3 _qgc_expected_regex)
    set(_qgc_test_name "GStreamer.${_qgc_script}")
    set(_qgc_test_args "-DSCRIPT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${_qgc_script}.cmake")
    if(_qgc_test_suffix)
        string(APPEND _qgc_test_name ".${_qgc_test_suffix}")
        list(APPEND _qgc_test_args "-DTEST_CASE=${_qgc_test_case}")
    endif()
    qgc_add_expected_failure_test(
        NAME
        "${_qgc_test_name}"
        EXPECTED_REGEX
        "${_qgc_expected_regex}"
        LABELS
        ${_qgc_gst_test_labels}
        TIMEOUT
        15
        ARGS
        ${_qgc_test_args}
    )
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
              version-format
              missing-mandatory
              missing-gl
              plugin-allowlist
              install
              install-macos
              android-mobile
              ios-link
              sdk-prefix-decoy
              homebrew
    )
        add_test(
            NAME "GStreamer.test_orchestrator.${_qgc_scenario}"
            COMMAND
                "${CMAKE_COMMAND}" "-DQGC_MODULE_DIR=${PROJECT_SOURCE_DIR}/cmake"
                "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/test_orchestrator/${_qgc_scenario}"
                "-DSCENARIO=${_qgc_scenario}" "-DGENERATOR=${CMAKE_GENERATOR}" "-DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/test_orchestrator.cmake"
        )
        set_tests_properties("GStreamer.test_orchestrator.${_qgc_scenario}"
                             PROPERTIES LABELS "${_qgc_gst_test_labels}" TIMEOUT 120 SKIP_REGULAR_EXPRESSION
                                        "QGC_TEST_SKIPPED"
        )
    endforeach()
endif()
