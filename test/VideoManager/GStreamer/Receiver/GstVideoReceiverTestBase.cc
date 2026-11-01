#include "GstVideoReceiverTestBase.h"

#ifdef QGC_GST_STREAMING

#include <QtTest/QTest>

bool GstVideoReceiverTestBase::runOnPipeline(GstVideoReceiver* receiver, const std::function<void(GstElement*)>& task)
{
    return receiver && receiver->_runOnPipeline(task);
}

GstElement* GstVideoReceiverTestBase::displaySink(const GstVideoReceiver* receiver)
{
    return receiver->_displaySink.get();
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
