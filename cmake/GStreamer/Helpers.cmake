# Aggregator entry-point for the cmake/GStreamer/ namespace. Consumers do
# `include(GStreamer/Helpers)` once and get the full helper surface. New code
# should include the focused submodule directly via
# `include("${CMAKE_CURRENT_LIST_DIR}/<Name>.cmake")` from another GStreamer/
# helper, or `include(GStreamer/<Name>)` from outside the namespace.
#
# Submodule contents:
#   Json         — _qgc_json_array_to_list (build-config.json reader)
#   Download     — URL templates, S3 mirror, checksum fetch,
#                  gstreamer_resilient_download (thin wrapper over
#                  project-level qgc_resilient_download in
#                  cmake/modules/Download.cmake) / gstreamer_download_sdk /
#                  qgc_gst_acquire_sdk (locked download + stamped extraction)
#   Components   — GSTREAMER_COMPONENT_REGISTRY,
#                  gstreamer_build_apis_and_deps, gstreamer_platform_plugin_attrs,
#                  gstreamer_scan_plugin_basenames
#   Link         — _gst_IGNORED_SYSTEM_LIBRARIES, _gst_SRT_REGEX_PATCH,
#                  _gst_save_find_suffixes / _gst_restore_find_suffixes,
#                  _gst_resolve_and_link_libraries
#   Layout       — gstreamer_create_layout_target (FLAT / FRAMEWORK /
#                  XCFRAMEWORK / STATIC_TARBALL → GStreamer::Layout)
#   Targets      — qgc_gst_create_pkgconfig_targets (GStreamer::GStreamer /
#                  deps / api_* / static plugin targets from pkg-config)
#   Probe        — qgc_check_gst_header (header/symbol check_cxx_source_compiles
#                  helper)
#   PkgConfig    — gstreamer_apply_pkgconfig_env (MODE SDK |
#                  SYSTEM_AUGMENT) — single PKG_CONFIG_PATH/LIBDIR
#                  mutator — and qgc_gst_restore_pkgconfig_env
#   PluginPolicy — gstreamer_plugins_for / gstreamer_filter_alternates /
#                  gstreamer_runtime_required_plugins / gstreamer_xcfw_skip /
#                  gstreamer_current_platform_key (alternate groups, runtime
#                  required, xcfw skip)
#   Install      — gstreamer_install_{plugins,gio_modules,libs} +
#                  gstreamer_install_{linux,windows,macos}_sdk +
#                  gstreamer_install_platform_sdk dispatcher
#   SdkState     — resolved-SDK cache entries, _qgc_gst_sdk_pc_version,
#                  fingerprint-based invalidation of cached probe results and
#                  cleanup of legacy cached pkg-config flags

include_guard(GLOBAL)

# Order matters: Install depends on Components (gstreamer_platform_plugin_attrs).
include("${CMAKE_CURRENT_LIST_DIR}/Json.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Components.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Link.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Layout.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Targets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Probe.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PkgConfig.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Download.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PluginPolicy.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Install.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/SdkState.cmake")
