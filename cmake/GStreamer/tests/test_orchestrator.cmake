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

# _qgc_write_fake_sdk(<root> <version> [WITH_PROBE_HEADERS] [OMIT <pc-module>...] [PLUGINS <plugin>...])
# WITH_PROBE_HEADERS adds the DMABuf and GstVideoOrientationMeta headers the Orchestrator probes for.
# Every library ships as .so and .a, and each plugin (default: coreelements) has a .pc file like in the
# static SDKs.
function(_qgc_write_fake_sdk root version)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "WITH_PROBE_HEADERS" "" "OMIT;PLUGINS")
    if(NOT ARG_PLUGINS)
        set(ARG_PLUGINS coreelements)
    endif()
    file(REMOVE_RECURSE "${root}")
    file(MAKE_DIRECTORY "${root}/include/gstreamer-1.0/gst")
    foreach(_plugin IN LISTS ARG_PLUGINS)
        file(WRITE "${root}/lib/gstreamer-1.0/libgst${_plugin}.so" "")
        file(WRITE "${root}/lib/gstreamer-1.0/libgst${_plugin}.a" "")
        file(WRITE "${root}/lib/pkgconfig/gst${_plugin}.pc"
             "prefix=${root}\n" "libdir=\${prefix}/lib/gstreamer-1.0\n\n" "Name: gst${_plugin}\n"
             "Description: QGC fake GStreamer plugin\n" "Version: ${version}\n" "Libs: -L\${libdir} -lgst${_plugin}\n"
        )
    endforeach()
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
        file(WRITE "${root}/lib/lib${_lib}.a" "")
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

# Installs the configured fixture with --prefix /opt/qgc into DESTDIR <WORK_DIR>/stage; sets _qgc_result and
# _qgc_output in the caller.
function(_qgc_install)
    file(REMOVE_RECURSE "${WORK_DIR}/stage")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "DESTDIR=${WORK_DIR}/stage" "${CMAKE_COMMAND}" --install "${_binary_dir}"
                --prefix /opt/qgc
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

# Sets <out_var> to the command line that links the fixture's shared library (Ninja or Makefile generators).
function(_qgc_link_line out_var)
    if(GENERATOR MATCHES "Ninja")
        file(READ "${_binary_dir}/build.ninja" _ninja)
        string(REGEX MATCH "\nbuild libQGCGStreamerFakeSdk\\.so: [^\n]*\n(  [^\n]*\n)*" _block "${_ninja}")
        set(_link "")
        foreach(_variable IN ITEMS LINK_FLAGS LINK_LIBRARIES)
            if(_block MATCHES "\n  ${_variable} = ([^\n]*)")
                string(APPEND _link " ${CMAKE_MATCH_1}")
            endif()
        endforeach()
    else()
        file(READ "${_binary_dir}/CMakeFiles/QGCGStreamerFakeSdk.dir/link.txt" _link)
    endif()
    set(${out_var}
        " ${_link} "
        PARENT_SCOPE
    )
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
    if(_qgc_output MATCHES "gstcoreelements")
        message(FATAL_ERROR "ASSERT [shared plugins]: a shared layout probed plugin pkg-config modules\n${_qgc_output}")
    endif()
elseif(SCENARIO STREQUAL "static-targets")
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_expect(_expect "${_sdk_a}" 1.24.0 OFF)
    _qgc_configure_ok("static targets" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" -DGStreamer_USE_STATIC_LIBS=ON
                      ${_expect}
    )
elseif(SCENARIO STREQUAL "legacy-pkg-config-args")
    # Older configures FORCE-cached the derived flags; a stale prefix would point pkg-config at the
    # wrong SDK, so only the user's own flags may survive.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_expect(_expect "${_sdk_a}" 1.24.0 OFF)
    file(WRITE "${WORK_DIR}/legacy-cache.cmake"
         "set(PKG_CONFIG_ARGN \"--short-errors;--static;--define-variable=prefix=${_sdk_b}\" CACHE STRING \"\")\n"
    )
    _qgc_configure_ok(
        "legacy cached flags"
        --fresh
        -C
        "${WORK_DIR}/legacy-cache.cmake"
        "-DGStreamer_ROOT_DIR=${_sdk_a}"
        -DEXPECT_CACHED_PKG_CONFIG_ARGN=--short-errors
        ${_expect}
    )
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
elseif(SCENARIO STREQUAL "android-mobile")
    if(NOT GENERATOR MATCHES "Ninja|Makefiles")
        message(STATUS "QGC_TEST_SKIPPED: link lines are only read from Ninja and Makefile builds")
        return()
    endif()
    # Mobile builds fail configure without the runtime-required plugins, so the SDK ships them; udp is left out
    # first to check that.
    include("${CMAKE_CURRENT_LIST_DIR}/../PluginPolicy.cmake")
    gstreamer_runtime_required_plugins(_plugins)
    list(REMOVE_ITEM _plugins udp)
    string(REPLACE ";" "," _requested "${_plugins};udp")
    set(_android -DQGC_TEST_ANDROID_ABI=arm64-v8a "-DQGC_TEST_PLUGINS=${_requested}")
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0 PLUGINS ${_plugins})
    file(WRITE "${_sdk_a}/share/gst-android/ndk-build/GStreamer.java" "")
    file(WRITE "${_sdk_a}/lib/gio/modules/libgioopenssl.a" "")
    foreach(_library IN ITEMS ssl crypto gio-2.0)
        file(WRITE "${_sdk_a}/lib/lib${_library}.a" "")
    endforeach()
    _qgc_configure_fails("Android SDK without udp"
                         "runtime-required plugins are missing from the static SDK .*: udp\\." --fresh
                         "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_android}
    )

    foreach(_suffix IN ITEMS so a)
        file(WRITE "${_sdk_a}/lib/gstreamer-1.0/libgstudp.${_suffix}" "")
    endforeach()
    file(READ "${_sdk_a}/lib/pkgconfig/gstcoreelements.pc" _pc)
    string(REPLACE "coreelements" "udp" _pc "${_pc}")
    file(WRITE "${_sdk_a}/lib/pkgconfig/gstudp.pc" "${_pc}")
    _qgc_configure_ok("Android mobile target" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_android})
    _qgc_link_line(_link)

    # The plugin archive is linked once, whole-archived; a plain occurrence ahead of it would load members twice.
    string(REGEX MATCHALL "[^ ]*libgstcoreelements\\.a" _plugin_archives "${_link}")
    qgc_test_assert_streq("plugin archive on the link line" "${_sdk_a}/lib/gstreamer-1.0/libgstcoreelements.a"
                          "${_plugin_archives}"
    )
    if(NOT _link MATCHES "--whole-archive +[^ ]*libgstcoreelements\\.a +[^ ]*(--pop-state|--no-whole-archive)")
        message(FATAL_ERROR "ASSERT [whole-archive]: the plugin archive is not whole-archived:\n${_link}")
    endif()
    if(_link MATCHES "--start-group|--end-group")
        message(FATAL_ERROR "ASSERT [link group]: lld needs no link group:\n${_link}")
    endif()

    # GStreamer::deps links the core's static dependencies with their symbols hidden from the app's exports.
    if(NOT _link MATCHES " ${_sdk_a}/lib/libgstreamer-1\\.0\\.a " OR NOT _link MATCHES
                                                                     "--exclude-libs,libgstreamer-1\\.0\\.a"
    )
        message(FATAL_ERROR "ASSERT [hidden core dependencies]: missing archive or --exclude-libs:\n${_link}")
    endif()
elseif(SCENARIO STREQUAL "install")
    # videoconvert+videoscale stand in for the fused videoconvertscale the verifier requires.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    foreach(_plugin IN ITEMS videoconvert videoscale)
        file(WRITE "${_sdk_a}/lib/gstreamer-1.0/libgst${_plugin}.so" "")
    endforeach()
    set(_plugins -DQGC_TEST_PLUGINS=coreelements,videoconvert,videoscale)
    _qgc_configure_ok("install rules" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_plugins}
                      -DQGC_TEST_INSTALL_REQUIRED_PLUGINS=coreelements,videoconvertscale
    )
    _qgc_install()
    if(NOT _qgc_result EQUAL 0 OR NOT _qgc_output MATCHES "all required plugins present in lib/gstreamer-1.0")
        message(FATAL_ERROR "ASSERT [install verified]: install failed or skipped the verifier\n${_qgc_output}")
    endif()
    foreach(_plugin IN ITEMS coreelements videoconvert videoscale)
        if(NOT EXISTS "${WORK_DIR}/stage/opt/qgc/lib/gstreamer-1.0/libgst${_plugin}.so")
            message(FATAL_ERROR "ASSERT [staged plugins]: libgst${_plugin}.so is not under DESTDIR/prefix")
        endif()
    endforeach()

    _qgc_configure_ok("install rules requiring udp" ${_plugins}
                      -DQGC_TEST_INSTALL_REQUIRED_PLUGINS=coreelements,videoconvertscale,udp
    )
    _qgc_install()
    string(REGEX REPLACE "[ \t\r\n]+" " " _normalized "${_qgc_output}")
    if(_qgc_result EQUAL 0 OR NOT _normalized MATCHES "missing required plugins in lib/gstreamer-1.0: udp ")
        message(FATAL_ERROR "ASSERT [missing plugin]: install did not fail for udp alone\n${_qgc_output}")
    endif()
else()
    message(FATAL_ERROR "Unknown SCENARIO: ${SCENARIO}")
endif()

qgc_test_pass("Orchestrator ${SCENARIO}")
