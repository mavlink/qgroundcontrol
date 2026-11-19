#include "GstQgcSinkControllerTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <memory>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>

#include "GStreamer.h"
#include "QGCQVideoSinkController.h"
#include "gstqgc/GstQgcTestSupport.h"

namespace {

enum class ControllerRelease
{
    OwnerDestroyed,
    VideoSinkDestroyed,
    Unbound,
};

/// The QVideoSink @p qvideosink delivers to, and whether it is active.
std::pair<gpointer, bool> sinkBinding(GstElement* qvideosink)
{
    gpointer videoSink = nullptr;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &videoSink, "active", &active, nullptr);
    return {videoSink, active != FALSE};
}

}  // namespace

void GstQgcSinkControllerTest::_testQVideoSinkControllerReleasesElement_data()
{
    QTest::addColumn<int>("release");

    QTest::newRow("owner destroyed") << int(ControllerRelease::OwnerDestroyed);
    QTest::newRow("QVideoSink destroyed") << int(ControllerRelease::VideoSinkDestroyed);
    QTest::newRow("QVideoSink unbound") << int(ControllerRelease::Unbound);
}

void GstQgcSinkControllerTest::_testQVideoSinkControllerReleasesElement()
{
    QFETCH(int, release);

    const GstQgcTest::SinkBin sink = GstQgcTest::makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const qvideosink = sink.qvideosink.get();

    auto videoSink = std::make_unique<QVideoSink>();
    auto controllerOwner = std::make_unique<QObject>();
    QVERIFY2(GStreamer::setupQVideoSinkElement(sink.bin.get(), videoSink.get(), controllerOwner.get()),
             "setupQVideoSinkElement() failed");
    QCOMPARE(sinkBinding(qvideosink), std::pair(static_cast<gpointer>(videoSink.get()), true));

    switch (static_cast<ControllerRelease>(release)) {
        case ControllerRelease::OwnerDestroyed:
            controllerOwner.reset();
            break;
        case ControllerRelease::VideoSinkDestroyed:
            videoSink.reset();
            QCoreApplication::sendPostedEvents(controllerOwner.get(), QEvent::MetaCall);
            break;
        case ControllerRelease::Unbound: {
            auto* const controller =
                controllerOwner->findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
            QVERIFY(controller);
            controller->setVideoSink(QPointer<QVideoSink>());
            break;
        }
    }
    const std::pair<gpointer, bool> released(nullptr, false);
    QCOMPARE(sinkBinding(qvideosink), released);

    // A controller that released its binding leaves the element alone when it goes.
    controllerOwner.reset();
    QCOMPARE(sinkBinding(qvideosink), released);
}

void GstQgcSinkControllerTest::_testQVideoSinkControllerNoWindowStartsInactive()
{
    const GstQgcTest::SinkBin sink = GstQgcTest::makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const qvideosink = sink.qvideosink.get();

    QObject controllerOwner;
    QGCQVideoSinkController controller(qvideosink, &controllerOwner);
    controller.setActive(true);

    QQuickVideoOutput videoOutput;
    QVERIFY(videoOutput.window() == nullptr);
    controller.followWindowVisibility(&videoOutput);

    QVERIFY(!sinkBinding(qvideosink).second);
}

void GstQgcSinkControllerTest::_testQVideoSinkControllerRebindRestoresActive_data()
{
    QTest::addColumn<bool>("active");
    QTest::addColumn<bool>("askedWhileUnbound");

    QTest::newRow("active") << true << false;
    QTest::newRow("inactive") << false << false;
    QTest::newRow("activated while unbound") << true << true;
    QTest::newRow("deactivated while unbound") << false << true;
}

void GstQgcSinkControllerTest::_testQVideoSinkControllerRebindRestoresActive()
{
    QFETCH(bool, active);
    QFETCH(bool, askedWhileUnbound);

    const GstQgcTest::SinkBin sink = GstQgcTest::makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    const auto elementActive = [&sink]() { return sinkBinding(sink.qvideosink.get()).second; };

    QVideoSink videoSink;
    QObject controllerOwner;
    QGCQVideoSinkController controller(sink.qvideosink.get(), &controllerOwner);
    controller.setVideoSink(QPointer<QVideoSink>(&videoSink));
    controller.setActive(askedWhileUnbound ? !active : active);

    // Without a QVideoSink the element delivers nothing, whatever is asked of it.
    controller.setVideoSink(QPointer<QVideoSink>());
    QVERIFY(!elementActive());
    if (askedWhileUnbound) {
        controller.setActive(active);
        QVERIFY(!elementActive());
    }

    // Bound again, it goes on as last asked, without a call to make it.
    controller.setVideoSink(QPointer<QVideoSink>(&videoSink));
    QCOMPARE(elementActive(), active);
}

void GstQgcSinkControllerTest::_testQVideoSinkControllerReleaseClearsVideoFrame()
{
    QVideoSink videoSink;
    GstQgcTest::SinkHarness harness(nullptr);
    QVERIFY(harness.isReady());
    QObject controllerOwner;
    auto* controller = new QGCQVideoSinkController(harness.element(), &controllerOwner);
    controller->setVideoSink(QPointer<QVideoSink>(&videoSink));

    QCOMPARE(harness.pushFrame(GST_SECOND), GST_FLOW_OK);
    QCoreApplication::sendPostedEvents(&videoSink, QEvent::MetaCall);
    QVERIFY(videoSink.videoFrame().isValid());

    // The released sink must not keep the last GstSample-backed frame alive.
    controller->prepareForRelease();
    QVERIFY(!videoSink.videoFrame().isValid());
}

#endif

UT_REGISTER_TEST(GstQgcSinkControllerTest, TestLabel::Unit)
