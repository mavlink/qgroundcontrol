cmake_minimum_required(VERSION 3.25)
# Configures fixtures/FakeSdk against generated fake GStreamer SDKs (stub headers, empty
# libraries and .pc files) to exercise the real Orchestrator on a Linux host.
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")

foreach(_required IN ITEMS QGC_MODULE_DIR WORK_DIR SCENARIO GENERATOR)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

find_program(_qgc_pkg_config NAMES pkg-config pkgconf NO_CACHE)
if(NOT _qgc_pkg_config)
    message(STATUS "QGC_TEST_SKIPPED: pkg-config is not installed")
    return()
endif()

set(_fixture_dir "${CMAKE_CURRENT_LIST_DIR}/fixtures/FakeSdk")
set(_binary_dir "${WORK_DIR}/build")
set(_sdk_a "${WORK_DIR}/sdk-a")
set(_sdk_b "${WORK_DIR}/sdk-b")
file(REMOVE_RECURSE "${WORK_DIR}")
# Linux discovery only prepends the SDK to PKG_CONFIG_PATH; hide the host's own .pc files so the
# fake SDK alone decides what is found.
file(MAKE_DIRECTORY "${WORK_DIR}/no-system-pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "${WORK_DIR}/no-system-pkgconfig")

# _qgc_write_fake_sdk(<root> <version> [WITH_PROBE_HEADERS] [OMIT <pc-module>...])
# WITH_PROBE_HEADERS adds the DMABuf and GstVideoOrientationMeta headers the Orchestrator probes for.
function(_qgc_write_fake_sdk root version)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "WITH_PROBE_HEADERS" "" "OMIT")
    file(REMOVE_RECURSE "${root}")
    file(MAKE_DIRECTORY "${root}/include/gstreamer-1.0/gst")
    file(WRITE "${root}/lib/gstreamer-1.0/libgstcoreelements.so" "")
    # pkg-config module|library
    set(_modules
        "gstreamer-1.0|gstreamer-1.0"
        "gstreamer-base-1.0|gstbase-1.0"
        "gstreamer-gl-1.0|gstgl-1.0"
        "gstreamer-gl-prototypes-1.0|gstgl-1.0"
        "gstreamer-rtsp-1.0|gstrtsp-1.0"
        "gstreamer-video-1.0|gstvideo-1.0"
        "gstreamer-app-1.0|gstapp-1.0"
        "gstreamer-allocators-1.0|gstallocators-1.0"
    )
    foreach(_module IN LISTS _modules)
        string(REPLACE "|" ";" _parts "${_module}")
        list(GET _parts 0 _pc)
        list(GET _parts 1 _lib)
        if(_pc IN_LIST ARG_OMIT)
            continue()
        endif()
        file(WRITE "${root}/lib/lib${_lib}.so" "")
        file(
            WRITE "${root}/lib/pkgconfig/${_pc}.pc"
            "prefix=${root}\n"
            "libdir=\${prefix}/lib\n"
            "includedir=\${prefix}/include\n\n"
            "Name: ${_pc}\n"
            "Description: QGC fake GStreamer SDK\n"
            "Version: ${version}\n"
            "Libs: -L\${libdir} -l${_lib}\n"
            "Cflags: -I\${includedir}/gstreamer-1.0\n"
        )
    endforeach()
    if(ARG_WITH_PROBE_HEADERS)
        file(WRITE "${root}/include/gstreamer-1.0/gst/allocators/gstdmabuf.h"
             "extern \"C\" int gst_is_dmabuf_memory(void *memory);\n"
        )
        file(WRITE "${root}/include/gstreamer-1.0/gst/video/gstvideometa.h"
             "typedef struct { int angle; } GstVideoOrientationMeta;\n"
        )
    endif()
endfunction()

# Configures the fixture with the extra <cmake-args>; sets _qgc_result and _qgc_output in the caller.
function(_qgc_configure)
    set(_command "${CMAKE_COMMAND}" -S "${_fixture_dir}" -B "${_binary_dir}" -G "${GENERATOR}")
    list(APPEND _command "-DQGC_MODULE_DIR=${QGC_MODULE_DIR}")
    if(MAKE_PROGRAM)
        list(APPEND _command "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}")
    endif()
    execute_process(
        COMMAND ${_command} ${ARGN}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _stdout
        ERROR_VARIABLE _stderr
    )
    set(_qgc_result
        "${_result}"
        PARENT_SCOPE
    )
    set(_qgc_output
        "${_stdout}\n${_stderr}"
        PARENT_SCOPE
    )
endfunction()

# Configures the fixture and fails the test, labelled <label>, unless configure succeeds.
function(_qgc_configure_ok label)
    _qgc_configure(${ARGN})
    if(NOT _qgc_result EQUAL 0)
        message(FATAL_ERROR "ASSERT [${label}]: configure failed\n${_qgc_output}")
    endif()
    set(_qgc_output
        "${_qgc_output}"
        PARENT_SCOPE
    )
endfunction()

# Configures the fixture and fails the test, labelled <label>, unless configure fails with <regex>.
function(_qgc_configure_fails label regex)
    _qgc_configure(${ARGN})
    string(REGEX REPLACE "[ \t\r\n]+" " " _normalized "${_qgc_output}")
    if(_qgc_result EQUAL 0)
        message(FATAL_ERROR "ASSERT [${label}]: configure unexpectedly succeeded\n${_qgc_output}")
    endif()
    if(NOT _normalized MATCHES "${regex}")
        message(FATAL_ERROR "ASSERT [${label}]: expected '${regex}'\n${_qgc_output}")
    endif()
endfunction()

# Returns the -D arguments that make the fixture assert it resolved <root> at <version>, with
# <probes> telling whether the SDK ships the probed headers.
function(_qgc_expect out_var root version probes)
    set(${out_var}
        "-DEXPECT_ROOT=${root}" "-DEXPECT_VERSION=${version}" "-DEXPECT_PROBES=${probes}"
        PARENT_SCOPE
    )
endfunction()

if(SCENARIO STREQUAL "targets")
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_expect(_expect "${_sdk_a}" 1.24.0 OFF)
    _qgc_configure_ok("targets" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_expect})
elseif(SCENARIO STREQUAL "idempotent")
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_configure_ok("first configure" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}")
    file(READ "${_binary_dir}/gst-state.txt" _first_state)
    _qgc_configure_ok("reconfigure")
    file(READ "${_binary_dir}/gst-state.txt" _second_state)
    qgc_test_assert_streq("state after reconfigure" "${_first_state}" "${_second_state}")
    if(_qgc_output MATCHES "Checking for module 'gstreamer-")
        message(FATAL_ERROR "ASSERT [cached probes]: unchanged SDK re-ran pkg-config probes\n${_qgc_output}")
    endif()
elseif(SCENARIO STREQUAL "reconfigure-root")
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_write_fake_sdk("${_sdk_b}" 1.26.0 WITH_PROBE_HEADERS)
    _qgc_expect(_expect "${_sdk_a}" 1.24.0 OFF)
    _qgc_configure_ok("SDK A" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_expect})
    _qgc_expect(_expect "${_sdk_b}" 1.26.0 ON)
    _qgc_configure_ok("switched to SDK B" "-DGStreamer_ROOT_DIR=${_sdk_b}" ${_expect})
elseif(SCENARIO STREQUAL "reconfigure-version")
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_expect(_expect "${_sdk_a}" 1.24.0 OFF)
    _qgc_configure_ok("SDK A 1.24" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_expect})
    _qgc_write_fake_sdk("${_sdk_a}" 1.26.0 WITH_PROBE_HEADERS)
    _qgc_expect(_expect "${_sdk_a}" 1.26.0 ON)
    _qgc_configure_ok("SDK A upgraded in place to 1.26" ${_expect})
elseif(SCENARIO STREQUAL "reuse-auto-root")
    # Linux never downloads, so stand in for a previous configure that auto-downloaded SDK A.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_expect(_expect "${_sdk_a}" 1.24.0 OFF)
    _qgc_configure_ok(
        "reused auto-downloaded root"
        --fresh
        "-DQGC_GST_RESOLVED_ROOT=${_sdk_a}"
        -DQGC_GST_RESOLVED_VERSION=1.20.0
        -DQGC_GST_RESOLVED_AUTO=TRUE
        ${_expect}
        -DEXPECT_AUTO=TRUE
    )
elseif(SCENARIO STREQUAL "missing-mandatory")
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0 OMIT gstreamer-rtsp-1.0)
    _qgc_configure_fails("SDK without a mandatory module"
                         "mandatory pkg-config modules not found .*: gstreamer-rtsp-1.0" --fresh
                         "-DGStreamer_ROOT_DIR=${_sdk_a}"
    )
elseif(SCENARIO STREQUAL "missing-gl")
    # A cross sysroot can lack gst-gl's x11 dependencies; only the optional GL GPU path needs it.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0 OMIT gstreamer-gl-1.0 gstreamer-gl-prototypes-1.0)
    _qgc_expect(_expect "${_sdk_a}" 1.24.0 OFF)
    _qgc_configure_ok("SDK without gst-gl" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_expect} -DEXPECT_GL=OFF)
else()
    message(FATAL_ERROR "Unknown SCENARIO: ${SCENARIO}")
endif()

qgc_test_pass("Orchestrator ${SCENARIO}")
