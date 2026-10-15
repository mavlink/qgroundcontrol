# Include aggregator for the cmake/GStreamer/ namespace (it defines nothing itself). Consumers do
# `include(GStreamer/Helpers)` once and get the full helper surface. New code should include the focused
# submodule directly via `include("${CMAKE_CURRENT_LIST_DIR}/<Name>.cmake")` from another GStreamer/ helper,
# or `include(GStreamer/<Name>)` from outside the namespace. Orchestrator.cmake (SDK discovery and target
# creation, included by the consumer) and platform/*.cmake (per-platform discovery, included by the
# Orchestrator) are not part of this aggregate.
#
# Submodule contents (leading-underscore names are internal to the namespace):
#   Json         — _qgc_gst_json_array_to_list (build-config.json array reader)
#   Platform     — qgc_gst_platform / qgc_gst_platform_descriptor (per-platform version variable,
#                  plugin key, discovery and install commands)
#   Download     — gstreamer_get_package_url / gstreamer_get_s3_mirror_url (URL templates),
#                  gstreamer_get_fallback_checksum / gstreamer_fetch_checksum (pins and sidecars),
#                  gstreamer_resilient_download (over qgc_resilient_download in
#                  cmake/modules/Download.cmake) / gstreamer_download_sdk /
#                  qgc_gst_acquire_sdk (locked download + stamped extraction)
#   Components   — GSTREAMER_COMPONENT_REGISTRY, gstreamer_resolve_component /
#                  gstreamer_mandatory_components / gstreamer_component_to_api /
#                  gstreamer_build_apis, gstreamer_platform_plugin_attrs /
#                  gstreamer_scan_plugin_basenames
#   Link         — _gst_IGNORED_SYSTEM_LIBRARIES, _gst_SRT_REGEX_PATCH,
#                  _gst_coalesce_existing_paths / _gst_recover_split_pkgconfig_paths (also
#                  used by Probe), _gst_strip_macos_absent_link_libs,
#                  _gst_save_find_suffixes / _gst_restore_find_suffixes,
#                  _gst_resolve_and_link_libraries
#   Layout       — gstreamer_create_layout_target (FLAT / FRAMEWORK /
#                  MERGED_STATIC / STATIC_TARBALL → GStreamer::Layout),
#                  gstreamer_layout_get / gstreamer_layout_set
#   Targets      — qgc_gst_create_pkgconfig_targets (GStreamer::GStreamer /
#                  deps / api_* / static plugin targets from pkg-config)
#   Probe        — qgc_check_gst_header (header/symbol check_cxx_source_compiles
#                  helper) and the optional-library probes HwBuffers uses
#   PkgConfig    — gstreamer_apply_pkgconfig_env (MODE SDK |
#                  SYSTEM_AUGMENT) — single PKG_CONFIG_PATH/LIBDIR
#                  mutator — and qgc_gst_restore_pkgconfig_env
#   PluginPolicy — GSTREAMER_PLUGIN_ALTERNATES / GSTREAMER_RUNTIME_REQUIRED_PLUGINS /
#                  GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS / GSTREAMER_MOCK_SERVER_DEBUG_PLUGINS,
#                  gstreamer_plugins_for / gstreamer_plugin_satisfy_sets /
#                  gstreamer_plugin_satisfied / gstreamer_missing_required /
#                  gstreamer_runtime_required_plugins / gstreamer_xcfw_skip
#   Install      — gstreamer_install_{plugins,gio_modules,libs},
#                  gstreamer_install_{linux,windows,macos}_sdk (macOS:
#                  gstreamer_install_macos_{framework,flat}_sdk) and the
#                  gstreamer_install_platform_sdk dispatcher + install-time verifier
#   StaticInit   — qgc_gst_generate_static_init (gst_init_static_plugins() source
#                  for static layouts, from gst_static_plugins.c.in)
#   SdkState     — resolved-SDK cache entries (_qgc_gst_select_sdk_root /
#                  _qgc_gst_record_resolved_sdk), _qgc_gst_sdk_pc_version,
#                  fingerprint-based invalidation of cached probe results
#                  (_qgc_gst_refresh_sdk_cache) and cleanup of legacy cached
#                  pkg-config flags (_qgc_gst_drop_legacy_pkg_config_args)

include_guard(GLOBAL)

# Order matters: Install depends on Components (gstreamer_platform_plugin_attrs).
include("${CMAKE_CURRENT_LIST_DIR}/Json.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Components.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Link.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Layout.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Targets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Probe.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PkgConfig.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Download.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PluginPolicy.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Install.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/StaticInit.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/SdkState.cmake")
