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
    Count,
};

/// Inputs the factory needs to build a GPU buffer. The layout is the same in every build: native handles are stored
/// as void* (EGLDisplay is a void* typedef, EGL_NO_DISPLAY is null), so no EGL or Vulkan header or QGC_HAS_* define is
/// needed here.
struct HwVideoBufferContext
{
    bool gpuEnabled = false;
    /// What the DMABuf path imports with now (HwBuffers::dmaBufImportDisplay()): Qt's EGLDisplay, or under Vulkan
    /// its VkPhysicalDevice. Null while it can't import.
    void* dmaBufImporter = nullptr;
    void* ahbEglDisplay = nullptr;  ///< EGLDisplay for the AHardwareBuffer path
};

namespace GstHw {

inline constexpr std::size_t kPathCount = static_cast<std::size_t>(HwVideoBufferPath::Count);

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
        case HwVideoBufferPath::Count:
            break;
    }
    return "Unknown";
}

}  // namespace GstHw
