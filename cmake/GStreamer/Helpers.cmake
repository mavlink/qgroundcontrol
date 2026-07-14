# Compatibility aggregator for callers that need the complete GStreamer helper
# surface. New code should include focused submodules directly.

include_guard(GLOBAL)

# Order matters: Install depends on Components (gstreamer_platform_plugin_attrs).
include("${CMAKE_CURRENT_LIST_DIR}/Json.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Components.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Link.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Layout.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Probe.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PkgConfig.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Download.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/PluginPolicy.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Install.cmake")
