#pragma once

#include <memory>

#include <QtCore/QString>

#include "GStreamerTestBase.h"

class QOffscreenSurface;
class QRhi;

/// GPU frames imported into a real GL QRhi and read back plane by plane: DMABuf planes through Qt's EGLImage importer,
/// and GLMemory from glupload through the GL bridge's shared context. Built on Linux with either path. Needs Qt to
/// render with EGL, which the offscreen QPA can't: CTest runs this class on xcb with xcb_egl, and every test skips when
/// Qt has no EGL context.
class GstHwBuffersGlReadbackTest : public GStreamerTestBase
{
    Q_OBJECT

public:
    GstHwBuffersGlReadbackTest();
    ~GstHwBuffersGlReadbackTest() override;

private slots:
    void init() override;
    void cleanupTestCase() override;

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    void _testNv12PlanesReadBack_data();
    void _testNv12PlanesReadBack();
    void _testVaDecodedFrameReadBack();
    void _testDmaBufBufferReturnsAfterGpuReads();
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    void _testGlMemoryFrameReadsBack_data();
    void _testGlMemoryFrameReadsBack();
    void _testGlMemoryFrameToImage_data();
    void _testGlMemoryFrameToImage();
    void _testTenBitPlanarFrameToImage();
    void _testGlMemoryMapLeavesFrameSyncMeta();
    void _testUnsharedGlMemoryTakesCpuPath();
    void _testGlBridgeKeepsGstGlInQtsApi();
#endif

private:
    /// A GL QRhi sharing Qt's EGL context, created once; null with the reason in _rhiUnavailable.
    QRhi* _glRhi();

    std::unique_ptr<QOffscreenSurface> _surface;
    std::unique_ptr<QRhi> _rhi;
    QString _rhiUnavailable;
    bool _rhiProbed = false;
};
