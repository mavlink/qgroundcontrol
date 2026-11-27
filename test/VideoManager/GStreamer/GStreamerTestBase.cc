#include "GStreamerTestBase.h"

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
}
