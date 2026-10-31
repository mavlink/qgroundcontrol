#pragma once

#include <cstddef>

/// GPU zero-copy path a frame is imported through; None means the frame takes the CPU path.
enum class HwVideoBufferPath
{
    None,
    DmaBuf,
    GlMemory,
    D3D11,
    D3D12,
    IOSurface,
    AHardwareBuffer,
    Vulkan,
    _Count,
};

/// Inputs the factory needs to build a GPU buffer. The layout is the same in every build: EGL displays are stored as
/// void* (EGLDisplay is a void* typedef, EGL_NO_DISPLAY is null), so no EGL header or QGC_HAS_* define is needed here.
struct HwVideoBufferContext
{
    bool gpuEnabled = false;
    void* dmaBufEglDisplay = nullptr;  ///< EGLDisplay for the DMABuf path
    void* ahbEglDisplay = nullptr;     ///< EGLDisplay for the AHardwareBuffer path
};

namespace GstHw {

inline constexpr std::size_t kPathCount = static_cast<std::size_t>(HwVideoBufferPath::_Count);

/// Label used in logs and stats. Kept out of the enum's namespace so QTest's unqualified toString() can't find it
/// through ADL for a QCOMPARE on HwVideoBufferPath (QTest delete[]s what toString() returns).
constexpr const char* toString(HwVideoBufferPath path) noexcept
{
    switch (path) {
        case HwVideoBufferPath::None:
            return "CPU";
        case HwVideoBufferPath::DmaBuf:
            return "DMABuf";
        case HwVideoBufferPath::GlMemory:
            return "GL";
        case HwVideoBufferPath::D3D11:
            return "D3D11";
        case HwVideoBufferPath::D3D12:
            return "D3D12";
        case HwVideoBufferPath::IOSurface:
            return "IOSurface";
        case HwVideoBufferPath::AHardwareBuffer:
            return "AHWBuf";
        case HwVideoBufferPath::Vulkan:
            return "Vulkan";
        case HwVideoBufferPath::_Count:
            break;
    }
    return "Unknown";
}

}  // namespace GstHw
