#include "HwBuffers/GstHwBuffersRhiCaptureTest.h"

#include <rhi/qrhi.h>

#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

#include "QGCRhiCapture.h"

void GstHwBuffersRhiCaptureTest::_testDevicePublishedAfterSceneGraphReinitialises()
{
    QQuickWindow window;
    if (QQuickWindow::sceneGraphBackend() == QLatin1String("software")) {
        QSKIP("Qt Quick renders in software here, which has no device to publish");
    }
#if QT_CONFIG(opengl)
    // Qt Quick aborts when the graphics API it is to render with doesn't start.
    QOpenGLContext openGl;
    if ((QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGL) && !openGl.create()) {
        QSKIP("No OpenGL here: the offscreen platform has it only with an X display");
    }
#endif
    // Hiding the window then releases its scene graph and its QRhi, and showing it again initialises both anew, as
    // Qt does after a device loss.
    window.setPersistentGraphics(false);
    window.setPersistentSceneGraph(false);
    window.resize(64, 64);
    QGCRhiCapture::connectWindow(&window);
    const auto publishedBackend = [] { return QGCRhiCapture::deviceSnapshot().backend.load(); };

    window.show();
    QTRY_COMPARE_WITH_TIMEOUT(publishedBackend(), static_cast<int>(QRhi::OpenGLES2), TestTimeout::mediumMs());

    window.hide();
    QTRY_COMPARE_WITH_TIMEOUT(publishedBackend(), -1, TestTimeout::mediumMs());

    // Qt's threaded render loop emits sceneGraphInitialized before it sets QQuickWindow::rhi() this time.
    window.show();
    QTRY_COMPARE_WITH_TIMEOUT(publishedBackend(), static_cast<int>(QRhi::OpenGLES2), TestTimeout::mediumMs());
}

UT_REGISTER_TEST(GstHwBuffersRhiCaptureTest, TestLabel::Integration)
