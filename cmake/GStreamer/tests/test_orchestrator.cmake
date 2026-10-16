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
    string(REPLACE "." ";" _parts "${version}")
    list(GET _parts 1 _minor)
    file(WRITE "${root}/include/gstreamer-1.0/gst/gstversion.h"
         "#define GST_VERSION_MAJOR (1)\n" "#define GST_VERSION_MINOR (${_minor})\n#define GST_VERSION_MICRO (0)\n"
    )
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
        qgc_test_split("${_module}" _pc _lib)
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

# _qgc_write_fake_android_sdk(<root> <version> <plugin>...)
# A fake SDK (see _qgc_write_fake_sdk) with the <plugin>s and what the Android SDK adds: the ndk-build anchor, the
# static OpenSSL GIO module and the archives it links.
function(_qgc_write_fake_android_sdk root version)
    _qgc_write_fake_sdk("${root}" ${version} PLUGINS ${ARGN})
    file(WRITE "${root}/share/gst-android/ndk-build/GStreamer.java" "")
    file(WRITE "${root}/lib/gio/modules/libgioopenssl.a" "")
    foreach(_library IN ITEMS ssl crypto gio-2.0)
        file(WRITE "${root}/lib/lib${_library}.a" "")
    endforeach()
endfunction()

# _qgc_write_fake_probe_lib(<root> <version>)
# Adds the optional gstreamer-qgcprobe-1.0 library the fixture probes like HwBuffers probes gst-d3d11 or gst-vulkan.
function(_qgc_write_fake_probe_lib root version)
    file(WRITE "${root}/include/gstreamer-1.0/gst/qgcprobe.h" "extern \"C\" int qgc_probe;\n")
    file(WRITE "${root}/lib/libgstqgcprobe-1.0.so" "")
    file(
        WRITE "${root}/lib/pkgconfig/gstreamer-qgcprobe-1.0.pc"
        "prefix=${root}\n"
        "libdir=\${prefix}/lib\n"
        "includedir=\${prefix}/include\n\n"
        "Name: gstreamer-qgcprobe-1.0\n"
        "Description: QGC fake optional GStreamer library\n"
        "Version: ${version}\n"
        "Libs: -L\${libdir} -lgstqgcprobe-1.0\n"
        "Cflags: -I\${includedir}/gstreamer-1.0\n"
    )
endfunction()

# _qgc_write_fake_homebrew(<prefix> [LINKED])
# A Homebrew prefix with brew, its own pkg-config (the host's, logging each call to <prefix>/pkg-config.log, with
# Homebrew's macOS shims as its default search path), and Cellar/{gstreamer/1.24.0,glib/2.84.0} kegs whose
# gstreamer-1.0.pc requires GLib, which requires the shims' libffi. Unlinked, opt/ points at both kegs; LINKED, their
# .pc files are linked into lib/pkgconfig and opt/ only has gstreamer.
function(_qgc_write_fake_homebrew prefix)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "LINKED" "" "")
    file(REMOVE_RECURSE "${prefix}")
    set(_keg "${prefix}/Cellar/gstreamer/1.24.0")
    set(_glib "${prefix}/Cellar/glib/2.84.0")
    _qgc_write_fake_sdk("${_keg}" 1.24.0)
    file(WRITE "${_keg}/lib/gstreamer-1.0/libgstcoreelements.dylib" "")
    file(APPEND "${_keg}/lib/pkgconfig/gstreamer-1.0.pc" "Requires: glib-2.0, gobject-2.0\n")
    set(_shims "${prefix}/Library/Homebrew/os/mac/pkgconfig/15")
    file(MAKE_DIRECTORY "${_glib}/include/glib-2.0" "${prefix}/opt" "${prefix}/lib/pkgconfig")
    file(WRITE "${_shims}/libffi.pc" "Name: libffi\nDescription: QGC fake shim\nVersion: 3.4\n")
    foreach(_module IN ITEMS glib-2.0 gobject-2.0)
        file(WRITE "${_glib}/lib/lib${_module}.so" "")
        file(WRITE "${_glib}/lib/pkgconfig/${_module}.pc"
             "prefix=${_glib}\n" "Name: ${_module}\n" "Description: QGC fake GLib\n" "Version: 2.84.0\n"
             "Libs: -L\${prefix}/lib -l${_module}\n" "Cflags: -I\${prefix}/include/glib-2.0\n"
        )
    endforeach()
    file(APPEND "${_glib}/lib/pkgconfig/gobject-2.0.pc" "Requires.private: libffi\n")
    file(CREATE_LINK ../Cellar/gstreamer/1.24.0 "${prefix}/opt/gstreamer" SYMBOLIC)
    if(ARG_LINKED)
        file(GLOB _pc_files "${_keg}/lib/pkgconfig/*.pc" "${_glib}/lib/pkgconfig/*.pc")
        foreach(_pc IN LISTS _pc_files)
            cmake_path(GET _pc FILENAME _name)
            file(CREATE_LINK "${_pc}" "${prefix}/lib/pkgconfig/${_name}" SYMBOLIC)
        endforeach()
    else()
        file(CREATE_LINK ../Cellar/glib/2.84.0 "${prefix}/opt/glib" SYMBOLIC)
    endif()
    set(_scripts "${prefix}-scripts")
    file(WRITE "${_scripts}/brew" "#!/bin/sh\necho \"${prefix}/opt/$2\"\n")
    file(
        WRITE "${_scripts}/pkg-config"
        "#!/bin/sh\necho \"$*\" >> \"${prefix}/pkg-config.log\"\n"
        "PKG_CONFIG_LIBDIR=\"\${PKG_CONFIG_LIBDIR-${_shims}}\"\n"
        "export PKG_CONFIG_LIBDIR\nexec \"${_qgc_pkg_config}\" \"$@\"\n"
    )
    file(
        COPY "${_scripts}/brew" "${_scripts}/pkg-config"
        DESTINATION "${prefix}/bin"
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )
endfunction()

# Fails the check named <label> unless the fixture resolved the gstreamer, and through it the GLib, of the fake Homebrew
# <prefix> with that prefix's pkg-config.
function(_qgc_assert_homebrew label prefix)
    file(READ "${_binary_dir}/gst-state.txt" _state)
    string(REGEX MATCH "GStreamer::GStreamer\\.INTERFACE_INCLUDE_DIRECTORIES=[^\n]*" _includes "${_state}")
    string(REGEX MATCH "GStreamer::deps\\.INTERFACE_LINK_LIBRARIES=[^\n]*" _libraries "${_state}")
    set(_glib "${prefix}/Cellar/glib/2.84.0")
    foreach(
        _case IN
        ITEMS "_state|GStreamer_ROOT_DIR=${prefix}/opt/gstreamer\n"
              "_state|GSTREAMER_PLUGIN_PATH=${prefix}/opt/gstreamer/lib/gstreamer-1.0\n"
              "_includes|${prefix}/Cellar/gstreamer/1.24.0/include/gstreamer-1.0"
              "_includes|${_glib}/include/glib-2.0"
              "_libraries|${_glib}/lib/libglib-2.0.so"
              "_libraries|${_glib}/lib/libgobject-2.0.so"
    )
        qgc_test_split("${_case}" _var _expected)
        string(FIND "${${_var}}" "${_expected}" _at)
        if(_at EQUAL -1)
            message(FATAL_ERROR "ASSERT [${label}]: '${_expected}' not in ${_var}:\n${_state}")
        endif()
    endforeach()
    qgc_test_assert_matches("${label} plugins" "version=1\\.24\\.0 path=shared .* plugins=1/1 " "${_qgc_output}")
    if(NOT EXISTS "${prefix}/pkg-config.log")
        message(FATAL_ERROR "ASSERT [${label}]: the Homebrew prefix's pkg-config was not used")
    endif()
    file(REMOVE "${prefix}/pkg-config.log")
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
    if(_qgc_result EQUAL 0)
        message(FATAL_ERROR "ASSERT [${label}]: configure unexpectedly succeeded\n${_qgc_output}")
    endif()
    qgc_test_assert_matches("${label}" "${regex}" "${_qgc_output}")
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

# _qgc_link_line(<out_var> [<output> <target>])
# Sets <out_var> to the command line that links <output>, the binary of <target> (default: the FakeSdk fixture's shared
# library), for Ninja or Makefile generators.
function(_qgc_link_line out_var)
    set(_output libQGCGStreamerFakeSdk.so)
    set(_target QGCGStreamerFakeSdk)
    if(ARGC EQUAL 3)
        set(_output "${ARGV1}")
        set(_target "${ARGV2}")
    endif()
    if(GENERATOR MATCHES "Ninja")
        file(READ "${_binary_dir}/build.ninja" _ninja)
        string(REPLACE "." "\\." _output "${_output}")
        string(REGEX MATCH "\nbuild ${_output}: [^\n]*\n(  [^\n]*\n)*" _block "${_ninja}")
        set(_link "")
        foreach(_variable IN ITEMS LINK_FLAGS LINK_LIBRARIES)
            if(_block MATCHES "\n  ${_variable} = ([^\n]*)")
                string(APPEND _link " ${CMAKE_MATCH_1}")
            endif()
        endforeach()
    else()
        file(READ "${_binary_dir}/CMakeFiles/${_target}.dir/link.txt" _link)
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

if(SCENARIO STREQUAL "reconfigure")
    # One build directory through an unchanged reconfigure, an in-place SDK upgrade and a switch to another SDK: the
    # cached probe results must follow every SDK change and only those.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_expect(_expect "${_sdk_a}" 1.24.0 OFF)
    _qgc_configure_ok("SDK A 1.24" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_expect})
    qgc_test_assert_not_matches("shared plugins" "gstcoreelements" "${_qgc_output}")
    file(READ "${_binary_dir}/gst-state.txt" _first_state)

    _qgc_configure_ok("unchanged reconfigure")
    file(READ "${_binary_dir}/gst-state.txt" _second_state)
    qgc_test_assert_streq("state after reconfigure" "${_first_state}" "${_second_state}")
    qgc_test_assert_not_matches("cached probes" "Checking for module 'gstreamer-" "${_qgc_output}")

    _qgc_write_fake_sdk("${_sdk_a}" 1.26.0 WITH_PROBE_HEADERS)
    _qgc_expect(_expect "${_sdk_a}" 1.26.0 ON)
    _qgc_configure_ok("SDK A upgraded in place to 1.26" ${_expect})

    # Same version, without the probed headers: only the root tells the SDKs apart.
    _qgc_write_fake_sdk("${_sdk_b}" 1.26.0)
    _qgc_expect(_expect "${_sdk_b}" 1.26.0 OFF)
    _qgc_configure_ok("switched to SDK B" "-DGStreamer_ROOT_DIR=${_sdk_b}" ${_expect})
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
elseif(SCENARIO STREQUAL "version-override")
    # QGC_GST_VERSION replaces build-config.json's version (here the fixture's 1.20.0 minimum).
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_configure_fails("SDK older than QGC_GST_VERSION" "gstreamer-1.0>=1.26.0" --fresh
                         "-DGStreamer_ROOT_DIR=${_sdk_a}" -DQGC_GST_VERSION=1.26.0
    )
elseif(SCENARIO STREQUAL "version-format")
    # A partial version would only fail later, as a missing checksum pin or a 404 on the SDK package.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_configure_fails("QGC_GST_VERSION without a micro version"
                         "QGC_GST_VERSION='1\\.24' is not a GStreamer release" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}"
                         -DQGC_GST_VERSION=1.24
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
    # Optional components are probed quietly and summed up in one status line.
    qgc_test_assert_not_matches("quiet optional probes" "Package 'gstreamer-gl-1\\.0'.* not found" "${_qgc_output}")
    qgc_test_assert_matches("optional components summary" "optional components not found: Gl, GlPrototypes( |$)"
                            "${_qgc_output}"
    )
elseif(SCENARIO STREQUAL "plugin-allowlist")
    # A misspelled allowlist name warns; a missing hardware-specific decoder only shows in a status line.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    _qgc_configure_ok("allowlist with a typo and an optional decoder" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}"
                      -DQGC_TEST_PLUGINS=coreelements,coreelemnts,qsv
    )
    qgc_test_assert_matches("allowlist warning"
                            "CMake Warning .* allowlist names not found as plugins in [^ ]+: coreelemnts Check"
                            "${_qgc_output}"
    )
    qgc_test_assert_matches("optional status" "-- GStreamer: optional decoder plugins not in [^ ]+: qsv \\("
                            "${_qgc_output}"
    )
elseif(SCENARIO STREQUAL "android-mobile")
    if(NOT GENERATOR MATCHES "Ninja|Makefiles")
        message(STATUS "QGC_TEST_SKIPPED: link lines are only read from Ninja and Makefile builds")
        return()
    endif()
    # Mobile builds fail configure without the runtime-required plugins, so the SDK ships them; udp is left out
    # first to check that.
    include("${CMAKE_CURRENT_LIST_DIR}/../PluginPolicy.cmake")
    set(_plugins ${GSTREAMER_RUNTIME_REQUIRED_PLUGINS})
    list(REMOVE_ITEM _plugins udp)
    list(APPEND _plugins androidmedia)
    string(REPLACE ";" "," _requested "${_plugins};udp")
    set(_package "${WORK_DIR}/android-package")
    set(_android -DQGC_TEST_ANDROID_ABI=arm64-v8a "-DQGC_TEST_PLUGINS=${_requested}"
                 "-DQGC_ANDROID_PACKAGE_SOURCE_DIR=${_package}"
    )
    _qgc_write_fake_android_sdk("${_sdk_a}" 1.24.0 ${_plugins})
    # source|path staged under the build directory
    set(_ndk_build "${_sdk_a}/share/gst-android/ndk-build")
    set(_java androidmedia/GstAhcCallback.java)
    set(_staged
        "${_ndk_build}/${_java}|extra_java_sources/org/freedesktop/gstreamer/${_java}"
        "${_ndk_build}/fontconfig/fonts.conf|extra_assets/fontconfig/fonts.conf"
        "${_ndk_build}/fontconfig/fonts/Ubuntu-R.ttf|extra_assets/fontconfig/fonts/truetype/Ubuntu-R.ttf"
        "${_sdk_a}/etc/ssl/certs/ca-certificates.crt|extra_assets/ssl/certs/ca-certificates.crt"
    )
    foreach(_entry IN LISTS _staged)
        qgc_test_split("${_entry}" _source)
        file(WRITE "${_source}" "${_source}\n")
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
    # Copies earlier builds wrote into the package sources would duplicate the staged files; other files stay.
    set(_legacy_copies "${_package}/src/org/freedesktop/gstreamer/androidmedia/GstAhcCallback.java"
                       "${_package}/assets/fontconfig/fonts.conf"
    )
    foreach(
        _file IN
        LISTS _legacy_copies
        ITEMS "${_package}/assets/keep.txt"
    )
        file(WRITE "${_file}" "")
    endforeach()
    _qgc_configure_ok("Android mobile target" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_android})
    _qgc_link_line(_link)

    # Configure stages the plugin Java sources and fonts/CA assets into the build directory, never the package sources.
    foreach(_entry IN LISTS _staged)
        qgc_test_split("${_entry}" _source _path)
        execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${_source}" "${_binary_dir}/${_path}"
                        RESULT_VARIABLE _different
        )
        qgc_test_assert_streq("staged ${_path}" 0 "${_different}")
    endforeach()
    file(
        GLOB_RECURSE _package_files
        RELATIVE "${_package}"
        "${_package}/*"
    )
    qgc_test_assert_streq("package sources" assets/keep.txt "${_package_files}")
    # So is the license notice of the GStreamer linked into the app.
    foreach(_file IN ITEMS NOTICE.txt LGPL-2.1.txt)
        if(NOT EXISTS "${_binary_dir}/extra_assets/licenses/gstreamer/${_file}")
            message(FATAL_ERROR "ASSERT [license notice]: ${_file} is not staged with the APK assets")
        endif()
    endforeach()

    # The plugin archive is linked once, whole-archived; a plain occurrence ahead of it would load members twice.
    string(REGEX MATCHALL "[^ ]*libgstcoreelements\\.a" _plugin_archives "${_link}")
    qgc_test_assert_streq("plugin archive on the link line" "${_sdk_a}/lib/gstreamer-1.0/libgstcoreelements.a"
                          "${_plugin_archives}"
    )
    qgc_test_assert_matches("whole-archive"
                            "--whole-archive +[^ ]*libgstcoreelements\\.a +[^ ]*(--pop-state|--no-whole-archive)"
                            "${_link}"
    )
    qgc_test_assert_not_matches("link group" "--start-group|--end-group" "${_link}")

    # GStreamer::deps links the core's static dependencies with their symbols hidden from the app's exports.
    if(NOT _link MATCHES " ${_sdk_a}/lib/libgstreamer-1\\.0\\.a " OR NOT _link MATCHES
                                                                     "--exclude-libs,libgstreamer-1\\.0\\.a"
    )
        message(FATAL_ERROR "ASSERT [hidden core dependencies]: missing archive or --exclude-libs:\n${_link}")
    endif()
elseif(SCENARIO STREQUAL "ios-link")
    # iOS (platform/IOS.cmake, built here from fixtures/IosLink): only GStreamer::mobile links the merged archive. A
    # core function QGC calls that the framework does not export fails the app's link, naming it, rather than linking
    # a second core into the app; QGC_GST_IOS_APP_CORE_SYMBOLS keeps such functions in the framework.
    if(NOT GENERATOR MATCHES "Ninja|Makefiles")
        message(STATUS "QGC_TEST_SKIPPED: link lines are only read from Ninja and Makefile builds")
        return()
    endif()
    set(_fixture_dir "${CMAKE_CURRENT_LIST_DIR}/fixtures/IosLink")
    _qgc_configure_ok("iOS targets")
    _qgc_link_line(_app_link qgc_ios_link_app qgc_ios_link_app)
    _qgc_link_line(_framework_link libgstreamer_mobile.so GStreamerMobileXcfw)
    qgc_test_assert_not_matches("app link line" "libGStreamer\\.a" "${_app_link}")
    qgc_test_assert_matches("app link line" "libgstreamer_mobile\\.so" "${_app_link}")
    qgc_test_assert_matches("framework link line" " ([^ ]*/)?sdk/libGStreamer\\.a " "${_framework_link}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${_binary_dir}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _output
    )
    if(_result EQUAL 0 OR NOT _output MATCHES "undefined (reference to|symbol:) [`']?gst_debug_bin_to_dot_file")
        message(FATAL_ERROR "ASSERT [unexported core function]: the app link does not fail naming "
                            "gst_debug_bin_to_dot_file (exit ${_result}):\n${_output}"
        )
    endif()

    _qgc_configure_ok("iOS targets keeping a core function" -DQGC_TEST_CORE_SYMBOLS=gst_debug_bin_to_dot_file)
    _qgc_link_line(_framework_link libgstreamer_mobile.so GStreamerMobileXcfw)
    qgc_test_assert_matches("-u" "-u,gst_debug_bin_to_dot_file" "${_framework_link}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${_binary_dir}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _output
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "ASSERT [kept core function]: the app does not link:\n${_output}")
    endif()
    find_program(_qgc_nm NAMES nm NO_CACHE)
    if(_qgc_nm)
        execute_process(COMMAND "${_qgc_nm}" -g --defined-only "${_binary_dir}/qgc_ios_link_app"
                        OUTPUT_VARIABLE _app_symbols COMMAND_ERROR_IS_FATAL ANY
        )
        qgc_test_assert_not_matches("single core" " gst_[a-z_]+( |$)" "${_app_symbols}")
    endif()

    # Functions of optional libraries (the Debug RTSP server) are kept by name once the framework exists.
    _qgc_configure_ok("iOS targets keeping a function by name" -DQGC_TEST_CORE_SYMBOLS=
                      -DQGC_TEST_KEEP_SYMBOLS=gst_debug_bin_to_dot_file
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${_binary_dir}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _output
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "ASSERT [qgc_gst_ios_keep_symbols]: the app does not link:\n${_output}")
    endif()
elseif(SCENARIO STREQUAL "sdk-prefix-decoy")
    # A bundled SDK locks pkg-config to itself: another GStreamer in CMAKE_PREFIX_PATH (vcpkg, Homebrew) must not
    # shadow it for the core targets or for the optional-library probes HwBuffers runs after discovery.
    include("${CMAKE_CURRENT_LIST_DIR}/../PluginPolicy.cmake")
    set(_plugins ${GSTREAMER_RUNTIME_REQUIRED_PLUGINS})
    string(REPLACE ";" "," _requested "${_plugins}")
    set(_decoy "${WORK_DIR}/decoy")
    _qgc_write_fake_android_sdk("${_sdk_a}" 1.24.0 ${_plugins})
    _qgc_write_fake_probe_lib("${_sdk_a}" 1.24.0)
    _qgc_write_fake_sdk("${_decoy}" 1.22.0)
    _qgc_write_fake_probe_lib("${_decoy}" 1.22.0)
    _qgc_configure_ok(
        "SDK with a decoy prefix"
        --fresh
        "-DGStreamer_ROOT_DIR=${_sdk_a}"
        -DQGC_TEST_ANDROID_ABI=arm64-v8a
        "-DQGC_TEST_PLUGINS=${_requested}"
        "-DCMAKE_PREFIX_PATH=${_decoy}"
        "-DEXPECT_PROBE_ROOT=${_sdk_a}"
        -DEXPECT_VERSION=1.24.0
    )
elseif(SCENARIO STREQUAL "homebrew")
    # macOS discovery of a Homebrew GStreamer, given as the prefix or opted into with QGC_GST_USE_HOMEBREW (brew found
    # on PATH): Homebrew's pkg-config, not an SDK lock-in, resolves the GLib and libffi gstreamer-1.0.pc requires.
    set(_brew "${WORK_DIR}/homebrew")
    set(_macos -DQGC_TEST_MACOS=ON)
    unset(ENV{PKG_CONFIG_LIBDIR})
    file(MAKE_DIRECTORY "${WORK_DIR}/path")
    file(CREATE_LINK "${_brew}/bin/brew" "${WORK_DIR}/path/brew" SYMBOLIC)
    set(ENV{PATH} "${WORK_DIR}/path:$ENV{PATH}")
    _qgc_write_fake_homebrew("${_brew}" LINKED)
    _qgc_configure_ok("Homebrew prefix as the root" --fresh "-DGStreamer_ROOT_DIR=${_brew}" ${_macos})
    _qgc_assert_homebrew("linked .pc files" "${_brew}")
    _qgc_write_fake_homebrew("${_brew}")
    _qgc_configure_ok("QGC_GST_USE_HOMEBREW" --fresh -DQGC_GST_USE_HOMEBREW=ON ${_macos})
    _qgc_assert_homebrew("unlinked kegs" "${_brew}")

    # An earlier auto-downloaded SDK stays ahead of Homebrew unless QGC_GST_USE_HOMEBREW is ON.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    file(WRITE "${_sdk_a}/lib/gstreamer-1.0/libgstcoreelements.dylib" "")
    set(_cached "-DQGC_GST_RESOLVED_ROOT:INTERNAL=${_sdk_a}" -DQGC_GST_RESOLVED_VERSION:INTERNAL=1.22.0
                -DQGC_GST_RESOLVED_AUTO:INTERNAL=TRUE
    )
    _qgc_configure_ok("cached SDK" --fresh ${_cached} ${_macos})
    qgc_test_assert_matches("cached SDK" "root=${_sdk_a} .*auto_downloaded=TRUE" "${_qgc_output}")
    _qgc_configure_ok("QGC_GST_USE_HOMEBREW over the cached SDK" --fresh ${_cached} -DQGC_GST_USE_HOMEBREW=ON ${_macos}
                      -DQGC_TEST_INSTALL_REQUIRED_PLUGINS=coreelements
    )
    _qgc_assert_homebrew("Homebrew over the cached SDK" "${_brew}")

    # Homebrew's libraries load each other by absolute paths, so an install cannot bundle them.
    _qgc_install()
    if(_qgc_result EQUAL 0)
        message(FATAL_ERROR "ASSERT [Homebrew install]: install succeeded\n${_qgc_output}")
    endif()
    qgc_test_assert_matches("Homebrew install" "Homebrew's GStreamer \\([^)]*\\), which cannot be bundled"
                            "${_qgc_output}"
    )
elseif(SCENARIO STREQUAL "install-macos")
    # The macOS bundle installers run on any host: everything must land under DESTDIR and the --prefix given at
    # install time, and nothing under the prefix known at configure time.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    foreach(
        _file IN
        ITEMS lib/libgstreamer-1.0.dylib lib/gstreamer-1.0/libgstcoreelements.dylib lib/gio/modules/libgioopenssl.dylib
              libexec/gstreamer-1.0/gst-plugin-scanner etc/ssl/certs/ca-certificates.crt
    )
        file(WRITE "${_sdk_a}/${_file}" "")
    endforeach()
    set(_framework "${WORK_DIR}/GStreamer.framework")
    foreach(_file IN ITEMS GStreamer lib/gstreamer-1.0/libgstcoreelements.dylib Resources/Info.plist)
        file(WRITE "${_framework}/Versions/1.0/${_file}" "")
    endforeach()
    set(_configured_prefix "-DCMAKE_INSTALL_PREFIX=${WORK_DIR}/configured-prefix")
    set(_bundle "${WORK_DIR}/stage/opt/qgc/QGCFake.app/Contents")

    _qgc_configure_ok("macOS framework install rules" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_configured_prefix}
                      "-DQGC_TEST_INSTALL_MACOS_FRAMEWORK=${_framework}"
    )
    _qgc_install()
    if(NOT _qgc_result EQUAL 0)
        message(FATAL_ERROR "ASSERT [framework install]: install failed\n${_qgc_output}")
    endif()
    set(_fw "${_bundle}/Frameworks/GStreamer.framework")
    foreach(_file IN ITEMS Versions/1.0/GStreamer Versions/1.0/lib/gstreamer-1.0/libgstcoreelements.dylib)
        if(NOT EXISTS "${_fw}/${_file}")
            message(FATAL_ERROR "ASSERT [framework install]: ${_file} is not under DESTDIR/prefix\n${_qgc_output}")
        endif()
    endforeach()
    foreach(_link IN ITEMS "Versions/Current|1.0" "GStreamer|Versions/Current/GStreamer"
                           "Resources|Versions/Current/Resources"
    )
        qgc_test_split("${_link}" _name _target)
        file(READ_SYMLINK "${_fw}/${_name}" _actual)
        qgc_test_assert_streq("framework symlink ${_name}" "${_target}" "${_actual}")
    endforeach()

    _qgc_configure_ok("macOS flat install rules" --fresh "-DGStreamer_ROOT_DIR=${_sdk_a}" ${_configured_prefix}
                      -DQGC_TEST_INSTALL_MACOS_FLAT=ON
    )
    _qgc_install()
    if(NOT _qgc_result EQUAL 0)
        message(FATAL_ERROR "ASSERT [flat install]: install failed\n${_qgc_output}")
    endif()
    foreach(
        _file IN
        ITEMS Frameworks/libgstreamer-1.0.dylib
              lib/gstreamer-1.0/libgstcoreelements.dylib
              lib/gio/modules/libgioopenssl.dylib
              libexec/gstreamer-1.0/gst-plugin-scanner
              Resources/etc/ssl/certs/ca-certificates.crt
              Resources/licenses/gstreamer/NOTICE.txt
              Resources/licenses/gstreamer/LGPL-2.1.txt
    )
        if(NOT EXISTS "${_bundle}/${_file}")
            message(FATAL_ERROR "ASSERT [flat install]: ${_file} is not under DESTDIR/prefix\n${_qgc_output}")
        endif()
    endforeach()
    # The rpath fixup visits the installed copies; without otool on this host it warns about each one.
    qgc_test_assert_matches("rpath fixup" "otool -l failed for ${_bundle}/lib/gstreamer-1.0/libgstcoreelements.dylib"
                            "${_qgc_output}"
    )
    if(EXISTS "${WORK_DIR}/configured-prefix" OR EXISTS "${WORK_DIR}/stage${WORK_DIR}/configured-prefix")
        message(FATAL_ERROR "ASSERT [install prefix]: files were installed under the configure-time prefix")
    endif()
elseif(SCENARIO STREQUAL "install")
    # videoconvert+videoscale stand in for the fused videoconvertscale the verifier requires.
    _qgc_write_fake_sdk("${_sdk_a}" 1.24.0)
    foreach(_plugin IN ITEMS videoconvert videoscale)
        file(WRITE "${_sdk_a}/lib/gstreamer-1.0/libgst${_plugin}.so" "")
    endforeach()
    # A system GIO module directory: the TLS/proxy/dconf modules are bundled, the VFS ones and the cache are not.
    set(_gio_bundled libgiognutls.so libgiolibproxy.so libgiognomeproxy.so libdconfsettings.so)
    foreach(
        _module IN
        LISTS _gio_bundled
        ITEMS libgvfsdbus.so libgioremote-volume-monitor.so giomodule.cache
    )
        file(WRITE "${_sdk_a}/lib/gio/modules/${_module}" "")
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
    file(
        GLOB _gio_staged
        RELATIVE "${WORK_DIR}/stage/opt/qgc/lib/gio/modules"
        "${WORK_DIR}/stage/opt/qgc/lib/gio/modules/*"
    )
    list(SORT _gio_staged)
    list(SORT _gio_bundled)
    qgc_test_assert_streq("staged GIO modules" "${_gio_bundled}" "${_gio_staged}")
    # The bundled GStreamer's license notice ships with it.
    file(READ "${WORK_DIR}/stage/opt/qgc/share/licenses/gstreamer/NOTICE.txt" _notice)
    if(NOT _notice MATCHES "^GStreamer 1\\.24\\.0\n"
       OR NOT EXISTS "${WORK_DIR}/stage/opt/qgc/share/licenses/gstreamer/LGPL-2.1.txt"
    )
        message(FATAL_ERROR "ASSERT [license notice]: NOTICE.txt or LGPL-2.1.txt not installed\n${_notice}")
    endif()

    _qgc_configure_ok("install rules requiring udp" ${_plugins}
                      -DQGC_TEST_INSTALL_REQUIRED_PLUGINS=coreelements,videoconvertscale,udp
    )
    _qgc_install()
    string(REGEX REPLACE "[ \t\r\n]+" " " _normalized "${_qgc_output}")
    if(_qgc_result EQUAL 0 OR NOT _normalized MATCHES "missing required plugins in lib/gstreamer-1.0: udp ")
        message(FATAL_ERROR "ASSERT [missing plugin]: install did not fail for udp alone\n${_qgc_output}")
    endif()

    # A distribution package (QGC_GST_BUNDLE_PLUGINS=OFF) installs nothing of GStreamer and verifies nothing.
    _qgc_configure_ok("install rules without bundled plugins" ${_plugins} -DQGC_GST_BUNDLE_PLUGINS=OFF)
    qgc_test_assert_matches("system plugins" "QGC_GST_BUNDLE_PLUGINS is OFF; the install uses the system's plugins"
                            "${_qgc_output}"
    )
    _qgc_install()
    if(NOT _qgc_result EQUAL 0 OR _qgc_output MATCHES "install verification")
        message(FATAL_ERROR "ASSERT [system plugins]: install failed or verified plugins\n${_qgc_output}")
    endif()
    if(EXISTS "${WORK_DIR}/stage")
        file(GLOB_RECURSE _staged "${WORK_DIR}/stage/*")
        qgc_test_assert_streq("nothing installed" "" "${_staged}")
    endif()
else()
    message(FATAL_ERROR "Unknown SCENARIO: ${SCENARIO}")
endif()

qgc_test_pass("Orchestrator ${SCENARIO}")
