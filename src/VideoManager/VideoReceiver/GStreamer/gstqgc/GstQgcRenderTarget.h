#pragma once

namespace GstQgc {

/// Graphics API Qt Quick renders the video with, which decides the GPU memory qgcqvideosink can import.
enum class RenderBackend
{
    Unknown,  ///< No scene graph yet, and the GUI thread hasn't recorded the requested API either.
    Null,     ///< QRhi's Null backend or the software renderer: no GPU import.
    OpenGL,
    Direct3D11,
    Direct3D12,
    Metal,
    Vulkan,
};

struct RenderTarget
{
    RenderBackend backend = RenderBackend::Unknown;
    /// Qt's OpenGL is known to run on EGL, so GStreamer's EGLImages (DMABuf, AHardwareBuffer) can use its display.
    /// False for GLX, WGL and CGL, and while that isn't known yet (no GL context, Android's platform plugin).
    bool egl = false;

    bool isOpenGlOnEgl() const { return (backend == RenderBackend::OpenGL) && egl; }

    /// gst-gl can share Qt's OpenGL context (GstGlContextBridge wraps an EGL or a GLX one), so GLMemory textures reach
    /// Qt directly. Qt's OpenGL on Linux and Android is always one of the two, even before it is known which.
    bool sharesGlContext() const noexcept;

    friend bool operator==(const RenderTarget&, const RenderTarget&) = default;
};

/// What qgcqvideosink renders into now; callable from any thread. The backend of QGC's scene graph once it runs
/// (QGCRhiCapture); before that, the API Qt Quick was asked to use, as last read on the GUI thread.
RenderTarget currentRenderTarget() noexcept;

const char* toString(RenderBackend backend) noexcept;

#ifdef QGC_UNITTEST_BUILD
/// currentRenderTarget() returns @p target while alive, then what it returned before.
class ScopedRenderTargetForTest
{
public:
    explicit ScopedRenderTargetForTest(RenderTarget target);
    ~ScopedRenderTargetForTest();
    ScopedRenderTargetForTest(const ScopedRenderTargetForTest&) = delete;
    ScopedRenderTargetForTest& operator=(const ScopedRenderTargetForTest&) = delete;

private:
    int _previous;
};
#endif

}  // namespace GstQgc
