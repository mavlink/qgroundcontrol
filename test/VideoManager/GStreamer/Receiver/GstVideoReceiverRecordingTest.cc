#include "GstVideoReceiverRecordingTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>

#include "BranchContext.h"

namespace {

using GstReceiverTest::FileFormat;
using GstReceiverTest::Gate;
using GstReceiverTest::kFinalizeTimeoutMs;
using GstReceiverTest::kFrameRate;
using GstReceiverTest::kMinRecordedFrames;
using GstReceiverTest::kRecordFrames;
using GstReceiverTest::kShortWatchdogTimeout;
using GstReceiverTest::Receiver;
using GstReceiverTest::State;
using GstReceiverTest::udpListenUri;

enum class RecordingEnd
{
    Stopped,
    RecorderError,
    Deactivated,
};

/// Why the recording at @p path isn't a finalized file, or an empty string when it is.
QString recordingProblem(const QString& path)
{
    QString failure;
    const int frames = GstTest::countPlayableFrames(path, failure);
    return (frames >= kMinRecordedFrames)
               ? QString()
               : QStringLiteral("%1: %2 frames recorded %3").arg(path).arg(frames).arg(failure);
}

/// The head (queue or valve) of the tee branch @p branch.
GStreamer::GstElementPtr branchHead(GstElement* pipeline, GStreamer::PipelineBranch branch, const char* factoryName)
{
    for (GStreamer::GstElementPtr& element : GstTest::childElements(GST_BIN(pipeline), /*recurse=*/true)) {
        GstElementFactory* const factory = gst_element_get_factory(element.get());
        if (factory && (g_strcmp0(gst_plugin_feature_get_name(factory), factoryName) == 0) &&
            (GStreamer::branchOf(GST_OBJECT(element.get())) == branch)) {
            return std::move(element);
        }
    }
    return nullptr;
}

}  // namespace

void GstVideoReceiverRecordingTest::_testDecoderFailureKeepsRecording()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    const QString path = tempDir.filePath(QStringLiteral("decoder-failure.mp4"));
    QVERIFY(stream.record(path));
    const int decodingChanges = stream.decodingSpy.count();

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Decoder error from .*injected decoder failure")));
    QVERIFY(runOnPipeline(stream.receiver.get(), [](GstElement* pipeline) {
        const GStreamer::GstElementPtr decoder = GstTest::findByFactory(pipeline, "decodebin3");
        GError* const error =
            g_error_new_literal(GST_STREAM_ERROR, GST_STREAM_ERROR_DECODE, "injected decoder failure");
        (void) gst_element_post_message(decoder.get(),
                                        gst_message_new_error(GST_OBJECT(decoder.get()), error, nullptr));
        g_error_free(error);
    }));

    // A new decoder takes over; the recording and the stream go on.
    QVERIFY_SIGNAL_COUNT_WAIT(stream.decodingSpy, decodingChanges + 2, TestTimeout::mediumMs());
    QVERIFY(stream.receiver->decoding());
    verifyExpectedLogMessage();
    QVERIFY(stream.framesSent(kRecordFrames));
    QCOMPARE(stream.receiver->state(), State::Streaming);
    QVERIFY(stream.recordingFinishedSpy.isEmpty());
    QVERIFY_TRUE_WAIT(stream.receiver->stats().decoderRestarts == 1, TestTimeout::mediumMs());
    QVERIFY(stream.receiver->stats().recordingMs >= 0);
    QVERIFY_TRUE_WAIT(stream.receiver->stats().recordingBytes > 0, TestTimeout::mediumMs());

    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QCOMPARE(stream.recordingStartedSpy.count(), 1);
    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testReconnectResumesRecordingInNextPart()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream(udpListenUri(), /*autoReconnect=*/true, kShortWatchdogTimeout);
    QVERIFY(stream.startStreaming());
    const QString path = tempDir.filePath(QStringLiteral("resumed.mp4"));
    QVERIFY(stream.record(path));

    // The camera drops out: the first part is closed and the recording waits for the stream to come back.
    stream.sender.stop();
    QVERIFY(stream.waitForState(State::Reconnecting));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QVERIFY(stream.receiver->recording());

    QVERIFY(stream.resumeSending());
    const QString part2 = tempDir.filePath(QStringLiteral("resumed_part2.mp4"));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.constLast().constFirst().toString(), part2);
    QVERIFY(stream.framesSent(kRecordFrames));
    // The recording's size counts the closed first part too.
    const qint64 part1Bytes = QFileInfo(path).size();
    QVERIFY(part1Bytes > 0);
    QVERIFY_TRUE_WAIT(stream.receiver->stats().reconnects == 1, TestTimeout::mediumMs());
    QVERIFY(stream.receiver->stats().recordingBytes >= part1Bytes);

    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), part2);
    QVERIFY(stream.errorSpy.isEmpty());
    for (const QString& file : {path, part2}) {
        const QString problem = recordingProblem(file);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }
}

void GstVideoReceiverRecordingTest::_testSlowRecordingDoesNotStallDisplay()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    // Opened before the receiver goes, so a failing test can't leave the recorder held.
    const auto gate = std::make_shared<Gate>();
    const auto releaseGate = qScopeGuard([gate] { gate->release(); });

    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // A small backlog, so storage that stops taking data shows quickly.
    constexpr guint kBacklogBytes = 16 * 1024;
    std::atomic<int> displayed{0};
    GStreamer::PadProbe displayProbe;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&displayed, &displayProbe](GstElement* pipeline) {
        const GStreamer::GstElementPtr queue = branchHead(pipeline, GStreamer::PipelineBranch::Recording, "queue");
        g_object_set(queue.get(), "max-size-bytes", kBacklogBytes, nullptr);
        const GStreamer::GstElementPtr valve = branchHead(pipeline, GStreamer::PipelineBranch::Decoding, "valve");
        const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(valve.get(), "src"));
        (void) displayProbe.add(valveSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &displayed);
    }));

    const QString path = tempDir.filePath(QStringLiteral("slow.mp4"));
    QVERIFY(stream.record(path));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording storage too slow")));
    // Whether the drained file closes before the finalize timeout depends on when the gate opens.
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording not finalized within")));
    QVERIFY(stream.blockRecorder(gate));

    // The backlog fills; the recording gives up on it instead of holding up the tee.
    const auto recorderLeaks = [&stream] {
        gint leaky = 0;
        (void) runOnPipeline(stream.receiver.get(), [&leaky](GstElement* pipeline) {
            const GStreamer::GstElementPtr queue = branchHead(pipeline, GStreamer::PipelineBranch::Recording, "queue");
            g_object_get(queue.get(), "leaky", &leaky, nullptr);
        });
        return leaky != 0;
    };
    QVERIFY_TRUE_WAIT(recorderLeaks(), TestTimeout::mediumMs());

    // The display keeps getting frames while the storage stays stuck.
    const int displayedBefore = displayed.load(std::memory_order_relaxed);
    QVERIFY_TRUE_WAIT(displayed.load(std::memory_order_relaxed) >= (displayedBefore + kFrameRate),
                      TestTimeout::mediumMs());
    QCOMPARE(stream.receiver->state(), State::Streaming);
    verifyExpectedLogMessage();

    // The file had frames, so its recordingFinished() carries the error, and nothing else does.
    gate->release();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!Receiver::lastFinished(stream.recordingFinishedSpy, 1).isEmpty());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    QVERIFY(stream.errorSpy.isEmpty());
    QCOMPARE(stream.receiver->state(), State::Streaming);
}

void GstVideoReceiverRecordingTest::_testDeactivateFinishesRecording()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());
    const QString path = tempDir.filePath(QStringLiteral("deactivated.mp4"));
    QVERIFY(stream.record(path));
    QVERIFY(stream.receiver->recording());

    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    QVERIFY(stream.waitForState(State::Stopped));

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    // Activating again doesn't bring the recording back.
    QVERIFY(stream.startStreaming());
    QVERIFY(!stream.receiver->recording());
    QCOMPARE(stream.recordingStartedSpy.count(), 1);
}

void GstVideoReceiverRecordingTest::_testRecordingPlayableWhileInProgress_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("suffix");

    QTest::newRow("mp4") << int(FileFormat::Mp4) << QStringLiteral("mp4");
    QTest::newRow("mov") << int(FileFormat::Mov) << QStringLiteral("mov");
}

void GstVideoReceiverRecordingTest::_testRecordingPlayableWhileInProgress()
{
    QFETCH(int, format);
    QFETCH(QString, suffix);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

    const QString path = tempDir.filePath(QStringLiteral("in-progress.") + suffix);
    QVERIFY(stream.record(path, static_cast<FileFormat>(format)));

    // A copy taken while recording is what a crash or power loss would leave on disk.
    const QString snapshot = tempDir.filePath(QStringLiteral("snapshot.") + suffix);
    QString failure;
    const bool playable = UnitTest::waitForCondition(
        [&] {
            (void) QFile::remove(snapshot);
            return QFile::copy(path, snapshot) && (GstTest::countPlayableFrames(snapshot, failure) > 0);
        },
        TestTimeout::mediumDuration(), u"mid-recording copy is playable");
    QVERIFY2(playable, qPrintable(failure));
}

void GstVideoReceiverRecordingTest::_testSecondRecordingIsNotTruncated()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

    const QString first = tempDir.filePath(QStringLiteral("first.mp4"));
    QVERIFY(stream.record(first));
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());

    const QString second = tempDir.filePath(QStringLiteral("second.mp4"));
    QVERIFY(stream.record(second));
    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());

    for (const QString& path : {first, second}) {
        const QString problem = recordingProblem(path);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }
}

void GstVideoReceiverRecordingTest::_testRecordingSwitchWithoutDisplay_data()
{
    QTest::addColumn<int>("ending");

    QTest::newRow("stopped") << int(RecordingEnd::Stopped);
    QTest::newRow("recorder error") << int(RecordingEnd::RecorderError);
    QTest::newRow("deactivated") << int(RecordingEnd::Deactivated);
}

void GstVideoReceiverRecordingTest::_testRecordingSwitchWithoutDisplay()
{
    QFETCH(int, ending);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    // Without a display sink the first recorder's EOS ends the whole pipeline too. That EOS can still be queued when
    // the second recording starts, and must not be taken for the second recorder's.
    Receiver stream;
    QVERIFY(stream.startStreaming());
    const QString first = tempDir.filePath(QStringLiteral("first.mp4"));
    const QString second = tempDir.filePath(QStringLiteral("second.mp4"));
    QVERIFY(stream.record(first));
    // Switched without a stop: the second file starts once the first one is finalized.
    QVERIFY(stream.record(second));
    QCOMPARE(stream.recordingFinishedSpy.count(), 1);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), first);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());

    QElapsedTimer clock;
    clock.start();
    switch (static_cast<RecordingEnd>(ending)) {
        case RecordingEnd::Stopped:
            stream.receiver->stopRecording();
            break;
        case RecordingEnd::RecorderError:
            expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                             QRegularExpression(QStringLiteral("^Recording error: .*injected recorder failure")));
            QVERIFY(runOnPipeline(stream.receiver.get(), [](GstElement* pipeline) {
                const GStreamer::GstElementPtr fileSink = GstTest::findByFactory(pipeline, "filesink");
                GError* const error =
                    g_error_new_literal(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_WRITE, "injected recorder failure");
                (void) gst_element_post_message(fileSink.get(),
                                                gst_message_new_error(GST_OBJECT(fileSink.get()), error, nullptr));
                g_error_free(error);
            }));
            break;
        case RecordingEnd::Deactivated:
            stream.receiver->setActive(false);
            break;
    }
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());
    QVERIFY2(clock.elapsed() < kFinalizeTimeoutMs,
             qPrintable(QStringLiteral("The second recording closed after %1 ms").arg(clock.elapsed())));
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), second);
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    // A file that opened reports its error through recordingFinished() only.
    QVERIFY(stream.errorSpy.isEmpty());

    if (static_cast<RecordingEnd>(ending) == RecordingEnd::RecorderError) {
        verifyExpectedLogMessage();
        QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QStringLiteral("injected recorder failure"));
        return;
    }
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    const QString problem = recordingProblem(second);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testStreamEndClosesRecordingAtOnce()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream(udpListenUri(), /*autoReconnect=*/true, kShortWatchdogTimeout);
    QVERIFY(stream.startStreaming());
    const QString path = tempDir.filePath(QStringLiteral("ended.mp4"));
    QVERIFY(stream.record(path));

    // The stream's EOS reaches the recorder before the receiver acts on the end of the stream, as an RTSP/TCP source's
    // EOS can. Sent past the source, which is cut off so only the watchdog ends the session, a second or two later.
    QElapsedTimer clock;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&clock](GstElement* pipeline) {
        const GStreamer::GstElementPtr tee = GstTest::findByFactory(pipeline, "tee");
        const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(gst_element_get_static_pad(tee.get(), "sink"));
        const GStreamer::GstPadPtr sourcePad = GStreamer::adoptPad(gst_pad_get_peer(teeSink.get()));
        (void) gst_pad_add_probe(
            sourcePad.get(), GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM,
            [](GstPad*, GstPadProbeInfo*, gpointer) { return GST_PAD_PROBE_DROP; }, nullptr, nullptr);
        clock.start();
        (void) gst_pad_send_event(teeSink.get(), gst_event_new_eos());
    }));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY2(clock.elapsed() < kFinalizeTimeoutMs,
             qPrintable(QStringLiteral("The recording closed after %1 ms").arg(clock.elapsed())));
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    // The closed file is the first part; the recording goes on in the next one once the stream is back.
    QVERIFY(stream.receiver->recording());
    QVERIFY(stream.resumeSending());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.constLast().constFirst().toString(),
             tempDir.filePath(QStringLiteral("ended_part2.mp4")));
    QCOMPARE(stream.recordingFinishedSpy.count(), 1);
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverRecordingTest::_testRecordingErrorKeepsStreaming()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

    // GStreamer adds follow-up errors (failed state change) whose number isn't ours to pin.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error: .*Could not open file")));
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error:")));
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("missing/unwritable.mp4")), FileFormat::Mp4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    verifyExpectedLogMessage();
    // Never opened, so never finished.
    QVERIFY(stream.recordingFinishedSpy.isEmpty());
    QCOMPARE(stream.receiver->state(), State::Streaming);

    const QString path = tempDir.filePath(QStringLiteral("recovered.mp4"));
    QVERIFY(stream.record(path));
    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testFailedRecordingStartLeavesBranchUsable()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

    // A child holding the recording sink's name makes the pipeline refuse the sink.
    GstElement* nameTaken = nullptr;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&nameTaken](GstElement* pipeline) {
        nameTaken = gst_bin_new("sinkbin");
        if (!gst_bin_add(GST_BIN(pipeline), nameTaken)) {
            nameTaken = nullptr;
        }
    }));
    QVERIFY(nameTaken);

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to add the recording sink")));
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("refused.mp4")), FileFormat::Mp4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    verifyExpectedLogMessage();

    QVERIFY(runOnPipeline(stream.receiver.get(),
                          [nameTaken](GstElement* pipeline) { (void) gst_bin_remove(GST_BIN(pipeline), nameTaken); }));
    QVERIFY(stream.record(tempDir.filePath(QStringLiteral("accepted.mp4"))));
}

void GstVideoReceiverRecordingTest::_testRecordingStartKeepsPipelinePlaying()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;

    // Read on the worker that changes it; "PAUSED->PLAYING" while a change is pending.
    const auto pipelineState = [&stream]() {
        QString state;
        (void) runOnPipeline(stream.receiver.get(), [&state](GstElement* pipeline) {
            GstState current = GST_STATE_VOID_PENDING;
            GstState pending = GST_STATE_VOID_PENDING;
            (void) gst_element_get_state(pipeline, &current, &pending, 0);
            state = QString::fromUtf8(gst_element_state_get_name(current));
            if (pending != GST_STATE_VOID_PENDING) {
                state += QStringLiteral("->") + QString::fromUtf8(gst_element_state_get_name(pending));
            }
        });
        return state;
    };

    QVERIFY(stream.startStreaming());
    QCOMPARE_TRUE_WAIT(pipelineState(), QStringLiteral("PLAYING"), TestTimeout::mediumMs());

    // The file sink can't preroll before the next keyframe; the live pipeline must not wait for it meanwhile.
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("recording.mp4")), FileFormat::Mp4);
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
}

void GstVideoReceiverRecordingTest::_testDestroyWhileRecordingFinalizesFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

    const QString path = tempDir.filePath(QStringLiteral("destroyed.mp4"));
    QVERIFY(stream.record(path));

    QElapsedTimer teardown;
    teardown.start();
    stream.receiver.reset();
    QVERIFY2(teardown.elapsed() < TestTimeout::mediumMs(),
             qPrintable(QStringLiteral("Destructor took %1 ms").arg(teardown.elapsed())));

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverRecordingTest::_testDestroyWithHungRecorderIsBounded()
{
    // The finalize timeout, then the receiver's margin for the pipeline to stop, then scheduling slack (more on CI).
    const qint64 kMaxDestructionMs =
        std::chrono::milliseconds(RecordingBranch::kFinalizeTimeout + std::chrono::seconds{2}).count() +
        (2 * TestTimeout::shortMs());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const auto gate = std::make_shared<Gate>();
    const auto releaseGate = qScopeGuard([gate] { gate->release(); });
    GWeakRef pipelineRef;
    g_weak_ref_init(&pipelineRef, nullptr);
    const auto clearRef = qScopeGuard([&pipelineRef] { g_weak_ref_clear(&pipelineRef); });

    {
        Receiver stream;
        QVERIFY(stream.startStreaming());
        QVERIFY(stream.record(tempDir.filePath(QStringLiteral("hung.mp4"))));
        g_weak_ref_set(&pipelineRef, stream.pipeline().get());
        // Storage that never returns, as a hung network mount.
        QVERIFY(stream.blockRecorder(gate));

        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Recording not finalized within")));
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Worker still busy after")));
        QElapsedTimer teardown;
        teardown.start();
        stream.receiver.reset();
        QVERIFY2(teardown.elapsed() < kMaxDestructionMs,
                 qPrintable(QStringLiteral("Destructor took %1 ms").arg(teardown.elapsed())));
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    }

    // The abandoned worker finishes on its own once the storage returns.
    gate->release();
    QVERIFY(UnitTest::waitForCondition(
        [&pipelineRef] {
            GstElement* const pipeline = static_cast<GstElement*>(g_weak_ref_get(&pipelineRef));
            if (pipeline) {
                gst_object_unref(pipeline);
            }
            return pipeline == nullptr;
        },
        TestTimeout::mediumDuration(), u"abandoned pipeline released"));
}

#endif

UT_REGISTER_TEST(GstVideoReceiverRecordingTest, TestLabel::Integration)
