cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../SdkState.cmake")

qgc_test_sandbox(_sandbox sdk-state)
file(MAKE_DIRECTORY "${_sandbox}/lib/pkgconfig" "${_sandbox}/auto-root")

file(WRITE "${_sandbox}/include/gstreamer-1.0/gst/gstversion.h"
     "#define GST_VERSION_MAJOR (1)\n#define GST_VERSION_MINOR (24)\n#define GST_VERSION_MICRO (9)\n"
)
_qgc_gst_header_version("${_sandbox}/include" _version)
qgc_test_assert_streq("header version" "1.24.9" "${_version}")
_qgc_gst_header_version("${_sandbox}/missing" _version)
qgc_test_assert_streq("no header" "" "${_version}")
qgc_test_pass("SDK version")

# cmake-lint: disable=C0103
set(QGC_GST_RESOLVED_PACKAGES
    "https://example.invalid/sdk.tar.xz|0123"
    CACHE INTERNAL ""
)
_qgc_gst_record_resolved_sdk("${_sandbox}/auto-root" 1.28.4 TRUE 1.28.4)
qgc_test_assert_streq("auto-downloaded SDK keeps its packages" "https://example.invalid/sdk.tar.xz|0123"
                      "$CACHE{QGC_GST_RESOLVED_PACKAGES}"
)
# requested version|user root|expected root|expected auto
set(_cases "1.28.4|${_sandbox}/user-root|${_sandbox}/user-root|FALSE" "1.28.4||${_sandbox}/auto-root|TRUE"
           "1.28.5|||FALSE"
)
foreach(_case IN LISTS _cases)
    qgc_test_split("${_case}" _requested _user_root _expected_root _expected_auto)
    _qgc_gst_select_sdk_root(_root _auto "${_user_root}" ${_requested})
    qgc_test_assert_streq("root for '${_case}'" "${_expected_root}" "${_root}")
    qgc_test_assert_streq("auto for '${_case}'" "${_expected_auto}" "${_auto}")
endforeach()

file(REMOVE_RECURSE "${_sandbox}/auto-root")
_qgc_gst_select_sdk_root(_root _auto "" 1.28.4)
qgc_test_assert_streq("vanished auto root" "" "${_root}")

_qgc_gst_record_resolved_sdk(/usr 1.28.4 FALSE 1.24.2)
_qgc_gst_select_sdk_root(_root _auto "" 1.28.4)
qgc_test_assert_streq("discovered root is rediscovered" "" "${_root}")
qgc_test_assert_streq("SDK version" 1.24.2 "$CACHE{QGC_GST_RESOLVED_SDK_VERSION}")
qgc_test_assert_streq("discovered SDK has no packages" "" "$CACHE{QGC_GST_RESOLVED_PACKAGES}")
qgc_test_pass("resolved SDK root selection")

set(_stale
    __pkg_config_checked_PC_GStreamer
    __pkg_config_arguments_PC_GStreamer_api_video
    PC_GStreamer_INCLUDE_DIRS
    pkgcfg_lib_PC_GStreamer_gstreamer-1.0
    _gst_arm64-v8a_gstreamer_1_0
    GStreamer_FW_CoreFoundation_LIB
    "QGC_GST_HAS_VIDEO_ORIENTATION_META_GStreamer::GStreamer"
    GST_D3D11_LIB
    QGC_GST_VULKAN_LIB
    _GST_gst_plugin_scanner_PROG
    _xcfw_foundation
)
set(_kept __pkg_config_checked_LIBURING PC_LibUSB_FOUND PKG_CONFIG_EXECUTABLE GStreamer_ROOT_DIR QGC_GST_RESOLVED_ROOT)
# User options share the QGC_GST_ / GStreamer_ prefixes and must survive an SDK change.
set(_gpu_paths
    DMABUF
    CUDA
    GLMEMORY
    IOSURFACE
    D3D11
    D3D12
    AHARDWAREBUFFER
    VULKAN
)
list(TRANSFORM _gpu_paths PREPEND QGC_GST_ENABLE_)
list(
    APPEND
    _kept
    ${_gpu_paths}
    QGC_GST_VERSION
    QGC_GST_CUDA_STUB_DIR
    QGC_GST_DOWNLOAD_TIMEOUT
    GStreamer_USE_STATIC_LIBS
    GStreamer_USE_FRAMEWORK
    GStreamer_REQUIRE_CHECKSUM
    GStreamer_DEBUG
)
# Returns <var>'s cache value, or "<unset>"; get_property handles names that ${} cannot (e.g. "::").
function(_qgc_test_cache_value var out_var)
    set(_value "<unset>")
    if(DEFINED CACHE{${var}})
        get_property(
            _value
            CACHE "${var}"
            PROPERTY VALUE
        )
    endif()
    set(${out_var}
        "${_value}"
        PARENT_SCOPE
    )
endfunction()

# Seeds every entry in _stale and _kept with "cached".
function(_qgc_test_seed_cache)
    foreach(_var IN LISTS _stale _kept)
        set(${_var}
            "cached"
            CACHE INTERNAL ""
        )
    endforeach()
endfunction()

# Asserts the _stale entries are <stale_state> and the _kept entries survived, after <label>.
function(_qgc_test_assert_cache label stale_state)
    foreach(_var IN LISTS _stale)
        _qgc_test_cache_value("${_var}" _value)
        qgc_test_assert_streq("${label}: ${_var}" "${stale_state}" "${_value}")
    endforeach()
    foreach(_var IN LISTS _kept)
        _qgc_test_cache_value("${_var}" _value)
        qgc_test_assert_streq("${label}: ${_var}" "cached" "${_value}")
    endforeach()
endfunction()

_qgc_test_seed_cache()
_qgc_gst_refresh_sdk_cache("sdk-a|1.24.0")
_qgc_test_assert_cache("first fingerprint" "<unset>")

_qgc_test_seed_cache()
_qgc_gst_refresh_sdk_cache("sdk-a|1.24.0")
_qgc_test_assert_cache("unchanged fingerprint" "cached")

_qgc_gst_refresh_sdk_cache("sdk-a|1.26.0")
_qgc_test_assert_cache("changed fingerprint" "<unset>")

qgc_test_pass("SDK fingerprint invalidates cached probe results")
