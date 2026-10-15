# Include aggregator for the cmake/GStreamer/ helper modules; it defines nothing itself. The GStreamer README
# (src/VideoManager/VideoReceiver/GStreamer/README.md, "Build Configuration") describes each module. Orchestrator.cmake
# and platform/*.cmake, which the Orchestrator includes, are not part of it.

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
