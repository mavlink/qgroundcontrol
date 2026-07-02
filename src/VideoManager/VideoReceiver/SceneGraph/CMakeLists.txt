# ============================================================================
# Scene-Graph Video Node
# Parked QSGRenderNode-based video item for HDR-passthrough (not wired into the
# live path). Depends on rhi/qrhi.h (GuiPrivate, linked at the src/ target level).
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCVideoNodeItem.cc
        QGCVideoNodeItem.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
