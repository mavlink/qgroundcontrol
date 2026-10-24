#include "GStreamerTestBase.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QRegularExpression>

#include "GStreamer.h"
#include "GStreamerLogging.h"
#include "GStreamerTestHelpers.h"

void GStreamerTestBase::initTestCase()
{
    UnitTest::initTestCase();

    GStreamer::redirectGLibLogging();
    if (GstTest::initGStreamer().isEmpty()) {
        QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");
    }
}

void GStreamerTestBase::init()
{
    const QString unavailable = GstTest::initGStreamer();
    if (!unavailable.isEmpty()) {
        QSKIP(qPrintable(unavailable));
    }

    UnitTest::init();

    static const QRegularExpression sGLibTypeRe(
        QStringLiteral("cannot register existing type|"
                       "g_type_add_interface_static.*G_TYPE_IS_INSTANTIATABLE|"
                       "g_once_init_leave.*result != 0"));
    // GStreamer/GLib type registration warnings are environment/startup-order dependent
    // and may occur 0..N times across test process lifetime.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtCriticalMsg, sGLibTypeRe);

    // On headless/software-GL CI the sink bin cannot honor construct-only GPU properties
    // and the GL bridge is disabled. These warnings are environment-dependent.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("gpu-zerocopy.*can't be set after construction")));
    ignoreLogMessage("Video.GStreamer.HwBuffers.GstGlBridge", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Qt GL context exposes neither EGL nor GLX")));
}

#else

void GStreamerTestBase::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif
