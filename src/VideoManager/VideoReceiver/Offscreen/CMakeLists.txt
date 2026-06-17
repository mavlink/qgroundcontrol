# ============================================================================
# Offscreen QML Compositor
# QQuickRenderControl-based offscreen renderer for PiP / headless compositing.
# Reusable component; not wired into the live UI. Depends on rhi/qrhi.h (GuiPrivate).
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCOffscreenRenderer.cc
        QGCOffscreenRenderer.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
