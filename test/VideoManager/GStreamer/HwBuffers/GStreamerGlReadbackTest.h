#pragma once

#include <memory>

#include <QtCore/QString>

#include "UnitTest.h"

class QOffscreenSurface;
class QRhi;

/// GPU frames imported into a real GL QRhi and read back plane by plane: DMABuf planes through Qt's EGLImage importer,
/// and GLMemory from glupload through the GL bridge's shared context. Needs Qt to render with EGL, which the offscreen
/// QPA can't: CTest runs this class on xcb with xcb_egl, and every test skips when Qt has no EGL context.
class GStreamerGlReadbackTest : public UnitTest
{
    Q_OBJECT

public:
    GStreamerGlReadbackTest();
    ~GStreamerGlReadbackTest() override;

private slots:
#ifdef QGC_GST_STREAMING
    void initTestCase() override;
    void init() override;
    void cleanupTestCase() override;

    void _testNv12PlanesReadBack_data();
    void _testNv12PlanesReadBack();
    void _testVaDecodedFrameReadBack();
    void _testGlMemoryFrameReadsBack();
    void _testGlMemoryFrameToImage();
#endif

private:
    /// A GL QRhi sharing Qt's EGL context, created once; null with the reason in _rhiUnavailable.
    QRhi* _glRhi();

    std::unique_ptr<QOffscreenSurface> _surface;
    std::unique_ptr<QRhi> _rhi;
    QString _rhiUnavailable;
    bool _rhiProbed = false;
};
