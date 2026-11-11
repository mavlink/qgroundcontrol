#include "GstVideoReceiverTestBase.h"

#ifdef QGC_GST_STREAMING

#include <QtTest/QTest>

#include "GstReceiverWorker.h"

bool GstVideoReceiverTestBase::runOnPipeline(GstVideoReceiver* receiver, const std::function<void(GstElement*)>& task)
{
    return receiver && receiver->_runOnPipeline(task);
}

GstElement* GstVideoReceiverTestBase::displaySink(const GstVideoReceiver* receiver)
{
    return receiver->_displaySink.get();
}

void GstVideoReceiverTestBase::setMaxRecordingPartDuration(GstVideoReceiver* receiver,
                                                           std::chrono::milliseconds duration)
{
    receiver->_postToWorker([duration](GstReceiverWorker& worker) { worker.setMaxRecordingPartDuration(duration); });
}

void GstVideoReceiverTestBase::initTestCase()
{
    GStreamerTestBase::initTestCase();
    if (QTest::currentTestFailed()) {
        return;
    }

    if (GstTest::initGStreamer().isEmpty()) {
        GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "mp4mux", "qtmux", "qtdemux");
    }
}

#endif
