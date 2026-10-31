#include "GstQgcSinkControllerTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>
#include <memory>

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>

#include "GStreamer.h"
#include "QGCQVideoSinkController.h"
#include "gstqgc/GstQgcTestSupport.h"

void GstQgcSinkControllerTest::_testQVideoSinkControllerClearsElementOnDestroy()
{
    const GstQgcTest::SinkBin sink = GstQgcTest::makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const bin = sink.bin.get();
    GstElement* const qvideosink = sink.qvideosink.get();

    QVideoSink videoSink;
    auto controllerOwner = std::make_unique<QObject>();
    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, &videoSink, controllerOwner.get()),
             "setupQVideoSinkElement() failed");

    gpointer installedSink = nullptr;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(&videoSink));
    QCOMPARE(active, TRUE);

    controllerOwner.reset();

    installedSink = &videoSink;
    active = TRUE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(nullptr));
    QCOMPARE(active, FALSE);
}

void GstQgcSinkControllerTest::_testQVideoSinkControllerClearsElementWhenVideoSinkDestroyed()
{
    const GstQgcTest::SinkBin sink = GstQgcTest::makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const bin = sink.bin.get();
    GstElement* const qvideosink = sink.qvideosink.get();

    auto videoSink = std::make_unique<QVideoSink>();
    QObject controllerOwner;
    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, videoSink.get(), &controllerOwner),
             "setupQVideoSinkElement() failed");

    gpointer installedSink = nullptr;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(videoSink.get()));
    QCOMPARE(active, TRUE);

    videoSink.reset();
    QCoreApplication::sendPostedEvents(&controllerOwner, QEvent::MetaCall);

    installedSink = reinterpret_cast<gpointer>(quintptr(0x1));
    active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(nullptr));
    QCOMPARE(active, FALSE);
}

void GstQgcSinkControllerTest::_testQVideoSinkControllerNullSinkStillDeactivatesOnDestroy()
{
    const GstQgcTest::SinkBin sink = GstQgcTest::makeSinkBin();
    QVERIFY2(sink.bin && sink.qvideosink, "qgcvideosinkbin or its qvideosink accessor failed");
    GstElement* const bin = sink.bin.get();
    GstElement* const qvideosink = sink.qvideosink.get();

    QVideoSink videoSink;
    auto controllerOwner = std::make_unique<QObject>();
    QVERIFY2(GStreamer::setupQVideoSinkElement(bin, &videoSink, controllerOwner.get()),
             "setupQVideoSinkElement() failed");

    auto* controller = controllerOwner->findChild<QGCQVideoSinkController*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(controller);

    controller->setVideoSink(QPointer<QVideoSink>());

    gpointer installedSink = &videoSink;
    gboolean active = FALSE;
    g_object_get(qvideosink, "qvideosink", &installedSink, "active", &active, nullptr);
    QCOMPARE(installedSink, static_cast<gpointer>(nullptr));
    QCOMPARE(active, FALSE);

    controllerOwner.reset();

    active = FALSE;
    g_object_get(qvideosink, "active", &active, nullptr);
    QCOMPARE(active, FALSE);
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

    gboolean active = TRUE;
    g_object_get(qvideosink, "active", &active, nullptr);
    QCOMPARE(active, FALSE);
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
