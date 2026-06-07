include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/cmake/QtRunCMakeTestHelpers.cmake)

if(CMAKE_VERSION VERSION_LESS 3.17.0)
    set(CMAKE_CURRENT_FUNCTION_LIST_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()

function(qt_internal_add_RunCMake_test test)
    # Add the common Qt specific setups
    set(common_args
        "-DQt6_DIR=${Qt6_DIR}"
        "-DCMAKE_MODULE_PATH=${CMAKE_CURRENT_FUNCTION_LIST_DIR}"
    )

    # Get test dir, like add_RunCMake_test does.
    if("${ARGV1}" STREQUAL "TEST_DIR")
      if("${ARGV2}" STREQUAL "")
        message(FATAL_ERROR "Invalid TEST_DIR value given.")
      endif()
      set(test_dir ${ARGV2})
    else()
      set(test_dir ${test})
    endif()

    # Get the path to the original file that add_RunCMake_test would run.
    set(script_path_to_include "${CMAKE_CURRENT_SOURCE_DIR}/${test_dir}/RunCMakeTest.cmake")

    # Create a wrapper script so we respect build test skip regex.
    set(testname "RunCMake.${test}")
    set(wrapper_file "${CMAKE_CURRENT_BINARY_DIR}/run_cmake_test_${testname}.cmake")

    string(JOIN "\n" pre_run_code ${_qt_internal_skip_build_test_pre_run})

    set(android_code "")
    if(ANDROID)
        qt_internal_get_android_cmake_policy_version_minimum_value(version)
        string(APPEND android_code "
# Avoid cmake policy deprecation warnings with older android NDKs appearing in stderr, which
# causes test failures if the test doesn't set
# set(RunCMake_TEST_OUTPUT_MERGE 1)
# to avoid stderr being polluted.
if(NOT QT_NO_SET_RUN_CMAKE_TESTS_CMAKE_POLICY_VERSION_MINIMUM)
    set(ENV{CMAKE_POLICY_VERSION_MINIMUM} ${version})
endif()")
    endif()

    _qt_internal_configure_file(CONFIGURE
        OUTPUT "${wrapper_file}"
        CONTENT "
${pre_run_code}
${android_code}
include(\"${script_path_to_include}\")
")

    # Let add_RunCMake_test know it should use the wrapper file via outer-scope variable.
    set(QT_RUN_CMAKE_SCRIPT_PATH "${wrapper_file}")
    add_RunCMake_test("${test}" ${common_args} ${ARGN})

    set_tests_properties("${testname}" PROPERTIES
        SKIP_REGULAR_EXPRESSION "${_qt_internal_skip_build_test_regex}")
endfunction()
