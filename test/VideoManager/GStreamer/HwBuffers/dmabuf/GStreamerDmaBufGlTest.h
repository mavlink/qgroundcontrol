#pragma once

#include <memory>

#include <QtCore/QString>

#include "UnitTest.h"

class QOffscreenSurface;
class QRhi;

/// DMABuf frames imported into a real GL QRhi and read back plane by plane. Needs Qt to render with EGL, which the
/// offscreen QPA can't: CTest runs this class on xcb with xcb_egl, and every test skips when Qt has no EGL display.
class GStreamerDmaBufGlTest : public UnitTest
{
    Q_OBJECT

public:
    GStreamerDmaBufGlTest();
    ~GStreamerDmaBufGlTest() override;

private slots:
#ifdef QGC_GST_STREAMING
    void initTestCase() override;
    void init() override;
    void cleanupTestCase() override;

    void _testNv12PlanesReadBack_data();
    void _testNv12PlanesReadBack();
    void _testVaDecodedFrameReadBack();
#endif

private:
    /// The GL QRhi on Qt's EGL display, created once; null with the reason in _rhiUnavailable.
    QRhi* _glRhi();

    std::unique_ptr<QOffscreenSurface> _surface;
    std::unique_ptr<QRhi> _rhi;
    QString _rhiUnavailable;
    bool _rhiProbed = false;
};
