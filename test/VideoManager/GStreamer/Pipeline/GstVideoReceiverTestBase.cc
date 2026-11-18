#include "GstVideoReceiverTestBase.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QDeadlineTimer>
#include <QtCore/QSemaphore>
#include <QtTest/QTest>

#include "GstReceiverWorker.h"

bool GstVideoReceiverTestBase::runOnPipeline(GstVideoReceiver* receiver, const std::function<void(GstElement*)>& task)
{
    // The wait for the worker to run the task.
    constexpr std::chrono::seconds kRunOnPipelineTimeout{10};

    if (!receiver) {
        return false;
    }

    // Not a BlockingQueuedConnection, which would wait for good on a worker that never gets to the task. The task
    // runs under the mutex, so once the wait is given up it is either done or never runs.
    struct Call
    {
        std::mutex mutex;
        bool abandoned = false;
        QSemaphore done;
    };

    const auto call = std::make_shared<Call>();
    GstReceiverWorker* const worker = receiver->_worker;
    const bool posted = QMetaObject::invokeMethod(
        worker,
        [worker, call, &task]() {
            const std::lock_guard lock(call->mutex);
            if (!call->abandoned) {
                task(worker->pipeline());
                call->done.release();
            }
        },
        Qt::QueuedConnection);
    if (!posted) {
        return false;
    }
    if (call->done.tryAcquire(1, QDeadlineTimer(kRunOnPipelineTimeout))) {
        return true;
    }
    const std::lock_guard lock(call->mutex);
    call->abandoned = true;
    return call->done.tryAcquire();
}

GstElement* GstVideoReceiverTestBase::displaySink(const GstVideoReceiver* receiver)
{
    return receiver->_displaySink.get();
}

void GstVideoReceiverTestBase::holdWorker(GstVideoReceiver* receiver,
                                          const std::shared_ptr<GstReceiverTest::Gate>& gate)
{
    receiver->_postToWorker([gate](GstReceiverWorker& /*worker*/) { gate->wait(); });
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
