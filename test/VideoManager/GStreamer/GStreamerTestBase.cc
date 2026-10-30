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
    GSTTEST_REQUIRE_GSTREAMER();

    UnitTest::init();

    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtCriticalMsg, GstTest::glibTypeRegistrationCriticals());
    // libproxy can't parse some host proxy bypass entries (e.g. "169.254/16" on macOS runners), and warns through
    // rtspsrc's proxy lookup.
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("\"pxbackend\" \"Could not parse ignore mask")));

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
