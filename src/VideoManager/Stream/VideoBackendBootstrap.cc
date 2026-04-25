#include "VideoBackendBootstrap.h"

#include "QGC.h"
#include "QGCApplication.h"

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#endif

#include <QtCore/QFuture>
#include <QtGui/QGuiApplication>
#include <QtQuick/QQuickWindow>

bool VideoBackendBootstrap::shouldSkipForUnitTests()
{
#ifdef QGC_GST_STREAMING
    return GStreamer::isAvailable() && qgcApp() && QGC::runningUnitTests()
           && !qEnvironmentVariableIsSet("QGC_TEST_ENABLE_GSTREAMER");
#else
    return false;
#endif
}

QFuture<bool> VideoBackendBootstrap::start(bool backendDisabledForUnitTests)
{
    if (backendDisabledForUnitTests)
        return QtFuture::makeReadyValueFuture(true);

    if (qApp->platformName() == QLatin1String("offscreen"))
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

#ifdef QGC_GST_STREAMING
    return GStreamer::initAsync();
#else
    return QtFuture::makeReadyValueFuture(true);
#endif
}
