cmake_minimum_required(VERSION 3.25)
# Runs the macOS SDK acquisition in script mode against pinned stand-ins for the official runtime and devel packages,
# which a stand-in pkgutil expands: the published tree is the merged flat root alone, without the expanded packages,
# the payloads' Headers directories and the static libraries.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Acquire)

if(CMAKE_HOST_WIN32)
    message(STATUS "QGC_TEST_SKIPPED: the stand-in pkgutil is a shell script")
    return()
endif()

qgc_test_sandbox(_sandbox macos-sdk-extract)
set(CPM_SOURCE_CACHE "${_sandbox}/cache")
set(QGC_GST_REQUESTED_VERSION 9.9.3)
set(_cache_dir "${CPM_SOURCE_CACHE}/gstreamer-mac-${QGC_GST_REQUESTED_VERSION}")

# Expands a package, here a tarball of what pkgutil --expand-full makes of one: a Payload directory per component.
file(WRITE "${_sandbox}/pkgutil"
     "#!/bin/sh\nmkdir -p \"$3\" && cd \"$3\" && exec \"${CMAKE_COMMAND}\" -E tar xf \"$2\"\n"
)
file(
    COPY "${_sandbox}/pkgutil"
    DESTINATION "${_sandbox}/bin"
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
)
set(ENV{PATH} "${_sandbox}/bin:$ENV{PATH}")

# <package>|<component>|<file in its payload>
foreach(
    _file IN
    ITEMS
        "gstreamer.pkg|core|lib/libgstreamer-1.0.0.dylib"
        "gstreamer.pkg|core|lib/gstreamer-1.0/libgstcoreelements.dylib"
        "gstreamer.pkg|core|libexec/gstreamer-1.0/gst-plugin-scanner"
        "gstreamer.pkg|net|lib/gio/modules/libgioopenssl.so"
        "gstreamer-devel.pkg|core-devel|Headers/gst/gst.h"
        "gstreamer-devel.pkg|core-devel|include/gstreamer-1.0/gst/gst.h"
        "gstreamer-devel.pkg|core-devel|lib/libgstreamer-1.0.a"
        "gstreamer-devel.pkg|core-devel|lib/gstreamer-1.0/libgstcoreelements.a"
        "gstreamer-devel.pkg|core-devel|lib/pkgconfig/gstreamer-1.0.pc"
)
    qgc_test_split("${_file}" _package _component _path)
    file(WRITE "${_sandbox}/src/${_package}/${_component}.pkg/Payload/${_path}" "")
endforeach()
file(MAKE_DIRECTORY "${_cache_dir}")
set(_checksums "")
foreach(_case IN ITEMS "gstreamer.pkg|macos" "gstreamer-devel.pkg|macos_devel")
    qgc_test_split("${_case}" _package _platform)
    file(
        GLOB _components
        RELATIVE "${_sandbox}/src/${_package}"
        "${_sandbox}/src/${_package}/*"
    )
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar cf "${_cache_dir}/${_package}" ${_components}
        WORKING_DIRECTORY "${_sandbox}/src/${_package}" COMMAND_ERROR_IS_FATAL ANY
    )
    file(SHA256 "${_cache_dir}/${_package}" _hash)
    list(APPEND _checksums "\"${_platform}\":\"${_hash}\"")
endforeach()
list(JOIN _checksums "," _checksums)
set(QGC_BUILD_CONFIG_CONTENT "{\"gstreamer\":{\"checksums\":{\"${QGC_GST_REQUESTED_VERSION}\":{${_checksums}}}}}")

_qgc_gst_acquire_macos_sdk(_root)
qgc_test_assert_streq("SDK root" "${_cache_dir}/sdk/root" "${_root}")
file(
    GLOB_RECURSE _published
    RELATIVE "${_cache_dir}/sdk"
    "${_cache_dir}/sdk/*"
)
list(SORT _published)
set(_expected
    .complete
    root/include/gstreamer-1.0/gst/gst.h
    root/lib/gio/modules/libgioopenssl.so
    root/lib/gstreamer-1.0/libgstcoreelements.dylib
    root/lib/libgstreamer-1.0.0.dylib
    root/lib/pkgconfig/gstreamer-1.0.pc
    root/libexec/gstreamer-1.0/gst-plugin-scanner
)
qgc_test_assert_streq("published tree" "${_expected}" "${_published}")
qgc_test_pass("macOS packages merge into a flat root without static libraries or the expanded packages")
