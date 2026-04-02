# ============================================================================
# USV Custom Build Overrides
# 无人船定制构建配置覆盖
# ============================================================================

# Override application name (optional - uncomment to customize)
# set(QGC_APP_NAME "USV Ground Control" CACHE STRING "Application Name" FORCE)
# set(QGC_APP_DESCRIPTION "Ground Control Station for Unmanned Surface Vehicles" CACHE STRING "Application Description" FORCE)

# Override organization info (optional - uncomment to customize)
# set(QGC_ORG_NAME "Your Company" CACHE STRING "Organization Name" FORCE)
# set(QGC_ORG_DOMAIN "yourcompany.com" CACHE STRING "Organization Domain" FORCE)

message(STATUS "QGC USV: Custom overrides loaded")

if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(QGC_VIEWER3D OFF CACHE BOOL "Disable 3D Viewer for Windows Debug USV builds" FORCE)
    message(STATUS "QGC USV: Viewer3D disabled for Windows Debug build to avoid MSVC C1060 on generated Qt resources")
endif()
