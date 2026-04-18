#include "VideoManagerIntegrationTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QRegularExpression>
#include <QtMultimedia/QVideoSink>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "FakeVideoReceiver.h"
#include "QGCVideoStreamInfo.h"
#include "SubtitleWriter.h"
#include "VideoFrameDelivery.h"
#include "VideoManager.h"
#include "VideoStream.h"
#include "VideoStreamModel.h"
#include "VideoStreamOrchestrator.h"

namespace {

// Drain pending events so async signal/slot flow settles.
void processEvents()
{
    QCoreApplication::processEvents();
}

}  // namespace

VideoManagerIntegrationTest::PrimedStream VideoManagerIntegrationTest::buildAndRegisterStream(VideoManager& mgr,
                                                                                              VideoStream::Role role,
                                                                                              const QString& uri)
{
    FakeVideoReceiver* fake = nullptr;
    auto factory = FakeReceiverHelpers::makeFactory(&fake);
    auto* stream = new VideoStream(role, factory, &mgr);
    // Per-role pointer caches were removed in favor of role lookup over
    // _streams; appending to the list is now the single registration point.
    auto* orch = mgr.streamOrchestrator();
    orch->_streams.append(stream);
    orch->_wireStreamSignals(stream);
    orch->_streamModel->addStream(stream);
    if (!uri.isEmpty())
        stream->setUri(uri);
    processEvents();
    return {stream, fake};
}

void VideoManagerIntegrationTest::init()
{
    UnitTest::init();

    // VideoManager construction touches SettingsManager → FactValueGrid,
    // which logs an uncategorized critical when it runs without a vehicle.
    // Not something this test exercises or cares about — suppress so the
    // UnitTest framework's cleanup check doesn't flag it as a failure.
    const QRegularExpression vehicleNullRx(QStringLiteral(".*vehicle is NULL.*"));
    expectLogMessage(QtCriticalMsg, vehicleNullRx);
    expectLogMessage(QtWarningMsg, vehicleNullRx);
    expectLogMessage(QtDebugMsg, vehicleNullRx);
    expectLogMessage(QtInfoMsg, vehicleNullRx);
    expectLogMessage(QtFatalMsg, vehicleNullRx);
}

// ─────────────────────────────────────────────────────────────────────────────
// Bridge fanout (Phase 1 #5, #8 + Qt #2 #3)
// ─────────────────────────────────────────────────────────────────────────────

void VideoManagerIntegrationTest::_testPrimaryBridgeChangedFiresOnBridgeSwap()
{
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    // bridgeChanged is now on VideoStream directly (VideoManager no longer
    // fans out per-role bridge signals — see #1/#2 migration).
    QSignalSpy spy(primary.stream, &VideoStream::bridgeChanged);

    QVideoSink sink;
    primary.stream->registerVideoSink(&sink);
    processEvents();
    QVERIFY2(spy.count() >= 1, "bridgeChanged must fire when the primary stream's bridge becomes available");

    // Bridge accessor still works via the stream.
    QVERIFY(primary.stream->bridge() != nullptr);
}

void VideoManagerIntegrationTest::_testThermalBridgeChangedFiresOnBridgeSwap()
{
    VideoManager mgr;
    auto thermal = buildAndRegisterStream(mgr, VideoStream::Role::Thermal);

    QSignalSpy spy(thermal.stream, &VideoStream::bridgeChanged);
    QVideoSink sink;
    thermal.stream->registerVideoSink(&sink);
    processEvents();

    QVERIFY2(spy.count() >= 1, "bridgeChanged must fire when the thermal stream's bridge becomes available");
    QVERIFY(thermal.stream->bridge() != nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Sink registration / role routing
// ─────────────────────────────────────────────────────────────────────────────

void VideoManagerIntegrationTest::_testRegisterVideoSinkRoutesByRole()
{
    // QML now calls stream->registerVideoSink() directly via streamForRole()
    // instead of going through VideoManager::registerVideoSink(). Verify the
    // streams accept and forward their sinks correctly.
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);
    auto thermal = buildAndRegisterStream(mgr, VideoStream::Role::Thermal);

    QVideoSink primarySink;
    QVideoSink thermalSink;

    primary.stream->registerVideoSink(&primarySink);
    thermal.stream->registerVideoSink(&thermalSink);
    processEvents();

    // Each stream's bridge should hold its corresponding sink, not the other.
    QVERIFY(primary.stream->bridge() != nullptr);
    QVERIFY(thermal.stream->bridge() != nullptr);
    QCOMPARE(primary.stream->bridge()->videoSink(), &primarySink);
    QCOMPARE(thermal.stream->bridge()->videoSink(), &thermalSink);
}

// ─────────────────────────────────────────────────────────────────────────────
// Model integration
// ─────────────────────────────────────────────────────────────────────────────

void VideoManagerIntegrationTest::_testStreamModelExposesPrimaryAndThermal()
{
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);
    auto thermal = buildAndRegisterStream(mgr, VideoStream::Role::Thermal);

    auto* model = mgr.streamModel();
    QVERIFY(model != nullptr);
    QCOMPARE(model->rowCount(), 2);

    // streamForRole returns the right pointers (Phase 1 Qt #4 added bounds
    // checking; here we verify the lookup itself works).
    QCOMPARE(model->streamForRole(static_cast<int>(VideoStream::Role::Primary)), primary.stream);
    QCOMPARE(model->streamForRole(static_cast<int>(VideoStream::Role::Thermal)), thermal.stream);

    // Out-of-range index returns nullptr (Phase 1 Qt #4 bounds check).
    QCOMPARE(model->streamForRole(-1), nullptr);
    QCOMPARE(model->streamForRole(99), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Aggregate state
// ─────────────────────────────────────────────────────────────────────────────

void VideoManagerIntegrationTest::_testAggregateDecodingFromStreams()
{
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);
    auto thermal = buildAndRegisterStream(mgr, VideoStream::Role::Thermal);

    QVERIFY(!mgr.decoding());

    // Drive primary into decoding via the fake receiver — VideoStream
    // delegates decoding() to receiver, manager aggregates from streams.
    primary.fake->forceDecoding(true);
    processEvents();
    QVERIFY(mgr.decoding());

    primary.fake->forceDecoding(false);
    processEvents();
    QVERIFY(!mgr.decoding());

    // Thermal decoding does NOT contribute to VideoManager::decoding() —
    // the aggregator explicitly filters out thermal streams because QML's
    // decoding property represents the primary video surface state.
    // Verify the filter by driving thermal decoding only.
    thermal.fake->forceDecoding(true);
    processEvents();
    QVERIFY2(!mgr.decoding(), "Thermal decoding must NOT count toward VideoManager::decoding()");
}

void VideoManagerIntegrationTest::_testDeferredSinkFlushedAfterStreamCreation()
{
    // With declarative sink routing, QML calls stream->registerVideoSink()
    // directly. The bridge is created eagerly in VideoStream's ctor; the
    // receiver is lazy (created on setUri). Registering a sink before a
    // receiver exists lands the sink on the bridge immediately — verify.
    // The "receiver inherits the sink on creation" half is covered by
    // `_testRegisterVideoSinkRoutesByRole` (which does set a URI).
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary, QString());

    // Bridge is always present after stream construction.
    QVERIFY(primary.stream->bridge() != nullptr);
    // No URI set → no receiver yet.
    QVERIFY(primary.stream->receiver() == nullptr);

    QVideoSink sink;
    primary.stream->registerVideoSink(&sink);
    processEvents();

    // Sink lands on the bridge immediately (there's no separate pending slot).
    QCOMPARE(primary.stream->pendingSink(), &sink);
    QCOMPARE(primary.stream->bridge()->videoSink(), &sink);
}

void VideoManagerIntegrationTest::_testUvcActivationTransfersSinkDeclaratively()
{
    // #16: activeStreamForRole(Primary) returns the UVC stream when UVC is active.
    // Simulates the QML Connections handler by connecting to activeStreamChanged in C++.
    VideoManager mgr;

    // Register primary and UVC streams.
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);
    FakeVideoReceiver* uvcFake = nullptr;
    auto* uvcStream = new VideoStream(VideoStream::Role::UVC, FakeReceiverHelpers::makeFactory(&uvcFake), &mgr);
    auto* orch = mgr.streamOrchestrator();
    orch->_streams.append(uvcStream);
    orch->_wireStreamSignals(uvcStream);
    orch->_streamModel->addStream(uvcStream);
    processEvents();

    // Initially (non-UVC), activeStreamForRole(Primary) returns the primary stream.
    auto* model = orch->_streamModel;
    QCOMPARE(model->activeStreamForRole(static_cast<int>(VideoStream::Role::Primary)), primary.stream);

    // Connect a C++ "Connections" handler that simulates what QML's onActiveStreamChanged does.
    VideoStream* lastActivePrimary = nullptr;
    (void)QObject::connect(model, &VideoStreamModel::activeStreamChanged, &mgr, [&](int role) {
        if (role == static_cast<int>(VideoStream::Role::Primary))
            lastActivePrimary = model->activeStreamForRole(role);
    });

    // Activate UVC — this should flip activeStreamForRole(Primary) to UVC stream
    // and emit activeStreamChanged(Primary).
    model->setUvcActive(true);
    processEvents();

    QCOMPARE(model->activeStreamForRole(static_cast<int>(VideoStream::Role::Primary)), uvcStream);
    QCOMPARE(lastActivePrimary, uvcStream);

    // Deactivate UVC — activeStreamForRole(Primary) returns primary again.
    model->setUvcActive(false);
    processEvents();

    QCOMPARE(model->activeStreamForRole(static_cast<int>(VideoStream::Role::Primary)), primary.stream);
    QCOMPARE(lastActivePrimary, primary.stream);
}

// ─────────────────────────────────────────────────────────────────────────────
// Headless frame-delivery harness (#7)
// ─────────────────────────────────────────────────────────────────────────────
//
// These tests push synthetic QVideoFrames through FakeVideoReceiver's bridge
// and verify the full appsink→bridge→QVideoSink path works without a display,
// GStreamer, or QMediaPlayer. `FakeVideoReceiver::deliverSyntheticFrame()`
// calls through frame delivery exactly the way real backends do
// (GstAppsinkBridge for GStreamer; QVideoSink::videoFrameChanged for QtMM/UVC).
//
// Test-thread layout: the FakeVideoReceiver and VideoFrameDelivery live on the
// main thread. deliverFrame is thread-safe from any caller; the frame is posted
// to the QVideoSink via a queued invokeMethod that runs on the sink's thread
// (main thread here), so tests must `processEvents()` after delivery for the
// sink to observe the frame.

void VideoManagerIntegrationTest::_testFrameDeliveryReachesSinkAndBridge()
{
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    QVideoSink sink;
    primary.stream->registerVideoSink(&sink);
    processEvents();

    QVERIFY(primary.stream->bridge() != nullptr);
    QCOMPARE(primary.stream->bridge()->videoSink(), &sink);

    const QSize expectedSize(1280, 720);
    QVERIFY(primary.fake->deliverSyntheticFrame(expectedSize));

    // Wait long enough for the queued sink->setVideoFrame invocation to land.
    QTRY_COMPARE(sink.videoFrame().size(), expectedSize);

    QCOMPARE(primary.stream->bridge()->frameCount(), quint64(1));
    QCOMPARE(primary.stream->bridge()->videoSize(), expectedSize);
    QCOMPARE(primary.stream->videoSize(), expectedSize);
    QCOMPARE(primary.stream->bridge()->droppedFrames(), quint64(0));

    // lastRawFrame round-trips the same frame (seqlock read).
    const QVideoFrame latest = primary.stream->bridge()->lastRawFrame();
    QCOMPARE(latest.size(), expectedSize);

    // Detach the sink so cleanup doesn't race on in-flight queued invocations
    // targeting the soon-to-be-destroyed sink.
    primary.stream->registerVideoSink(nullptr);
    processEvents();
}

void VideoManagerIntegrationTest::_testAnnouncedFormatUpdatesVideoSizeBeforeFirstFrame()
{
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    QVideoSink sink;
    primary.stream->registerVideoSink(&sink);
    processEvents();

    const QSize announced(1920, 1080);
    primary.fake->announceFormat(announced);

    // announceFormat dispatches through the delivery mutator funnel; give it a tick.
    QTRY_COMPARE(primary.stream->bridge()->videoSize(), announced);

    QCOMPARE(primary.stream->bridge()->frameCount(), quint64(0));  // no real frame yet

    primary.stream->registerVideoSink(nullptr);
    processEvents();
}

void VideoManagerIntegrationTest::_testBackpressureDropsExcessFrames()
{
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    QVideoSink sink;
    primary.stream->registerVideoSink(&sink);
    processEvents();

    // Burst-deliver more frames than the delivery in-flight limit without
    // processing events in between. The first 3 enter the in-flight queue;
    // everything beyond that hits the backpressure drop path.
    constexpr int kBurst = 20;
    const int issued = primary.fake->deliverSyntheticFrames(kBurst);
    QCOMPARE(issued, kBurst);

    // Drop counter is updated synchronously on the delivering thread, so we
    // can assert without processEvents — no queue hop before the drop.
    const quint64 dropped = primary.stream->bridge()->droppedFrames();
    QVERIFY2(dropped > 0, qPrintable(QStringLiteral("Expected drops under backpressure, got %1").arg(dropped)));
    // Accepted + dropped == issued (no frames vanish silently).
    QCOMPARE(primary.stream->bridge()->frameCount() + dropped, quint64(kBurst));

    // Detach sink and drain queued frame-delivery invocations BEFORE the
    // local sink goes out of scope.
    primary.stream->registerVideoSink(nullptr);
    processEvents();
    processEvents();
}

void VideoManagerIntegrationTest::_testWatchdogFiresAfterSilence()
{
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    QVideoSink sink;
    primary.stream->registerVideoSink(&sink);
    processEvents();

    // Prime the watchdog with one frame so `_lastFrameMs` is non-sentinel —
    // otherwise the bridge's first tick fires immediately against the -1
    // default and the timing assertion below becomes meaningless.
    QVERIFY(primary.fake->deliverSyntheticFrame(QSize(640, 480)));
    processEvents();

    using namespace std::chrono_literals;

    // Observe via a bool flag so we don't outlive QSignalSpy through cleanup.
    bool fired = false;
    const auto conn = QObject::connect(primary.stream->bridge(), &VideoFrameDelivery::watchdogTimeout,
                                       &mgr, [&fired]() { fired = true; });

    primary.stream->bridge()->armWatchdog(100ms);

    QTRY_VERIFY_WITH_TIMEOUT(fired, 2000);

    primary.stream->bridge()->disarmWatchdog();
    QObject::disconnect(conn);

    primary.stream->registerVideoSink(nullptr);
    processEvents();
}

// ─────────────────────────────────────────────────────────────────────────────
// Stream-swap regressions (sink re-registration, bridge cleanup, watchdog arm)
// ─────────────────────────────────────────────────────────────────────────────

void VideoManagerIntegrationTest::_testRegisterNullSinkClearsBridge()
{
    // Contract: QGCVideoOutput.qml deregisters on role swap by calling
    // registerVideoSink(null). That must leave the bridge with no sink so the
    // old stream stops pushing setVideoFrame() once a new stream owns output.
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    QVideoSink sink;
    primary.stream->registerVideoSink(&sink);
    processEvents();
    QCOMPARE(primary.stream->bridge()->videoSink(), &sink);

    primary.stream->registerVideoSink(nullptr);
    processEvents();
    QCOMPARE(primary.stream->bridge()->videoSink(), nullptr);
}

void VideoManagerIntegrationTest::_testSwapSinkBetweenStreamsClearsOldBridge()
{
    // Regression for the pre-audit QGCVideoOutput.qml behaviour: on
    // activeStreamChanged(Primary), it registered the sink with the new
    // stream without clearing the old stream's bridge. Two bridges then
    // held the same sink pointer and raced on setVideoFrame. The fix
    // added _registeredStream tracking + deregister-before-register in
    // _rebindSink(). Codify the invariant at the C++ layer: after a
    // clean swap, only the new stream's bridge has the sink.
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    FakeVideoReceiver* uvcFake = nullptr;
    auto* uvcStream = new VideoStream(VideoStream::Role::UVC,
                                      FakeReceiverHelpers::makeFactory(&uvcFake), &mgr);
    auto* orch = mgr.streamOrchestrator();
    orch->_streams.append(uvcStream);
    orch->_wireStreamSignals(uvcStream);
    orch->_streamModel->addStream(uvcStream);
    uvcStream->setUri(QStringLiteral("uvc://local"));
    processEvents();

    QVideoSink sink;
    primary.stream->registerVideoSink(&sink);
    processEvents();
    QCOMPARE(primary.stream->bridge()->videoSink(), &sink);

    // Swap: mimic the QGCVideoOutput _rebindSink() flow.
    primary.stream->registerVideoSink(nullptr);
    uvcStream->registerVideoSink(&sink);
    processEvents();

    QVERIFY2(primary.stream->bridge()->videoSink() == nullptr,
             "Old stream's bridge must no longer reference the sink after swap");
    QCOMPARE(uvcStream->bridge()->videoSink(), &sink);

    // And back — deactivating UVC returns the sink to Primary.
    uvcStream->registerVideoSink(nullptr);
    primary.stream->registerVideoSink(&sink);
    processEvents();

    QVERIFY(uvcStream->bridge()->videoSink() == nullptr);
    QCOMPARE(primary.stream->bridge()->videoSink(), &sink);

    primary.stream->registerVideoSink(nullptr);
    processEvents();
}

void VideoManagerIntegrationTest::_testWatchdogDoesNotFireBeforeFirstFrame()
{
    // Regression for the pre-audit VideoFrameDelivery::_applyArmWatchdog which
    // primed `_lastFrameMs = now`. HW-decoder negotiation (vah264dec/vah265dec/
    // v4l2h264dec) can spend 1–3 s before the first sample — the pre-stamp
    // caused spurious watchdogTimeout on every cold connect. The fix leaves
    // `_lastFrameMs` at the resetStats-seeded -1 so _onWatchdogTick returns
    // early until the first real frame lands.
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    QVideoSink sink;
    primary.stream->registerVideoSink(&sink);
    processEvents();

    using namespace std::chrono_literals;

    bool fired = false;
    const auto conn = QObject::connect(primary.stream->bridge(), &VideoFrameDelivery::watchdogTimeout,
                                       &mgr, [&fired]() { fired = true; });

    primary.stream->bridge()->resetStats();  // matches validateBridgeForDecoding flow
    primary.stream->bridge()->armWatchdog(100ms);

    // Wait past the 500 ms tick cadence × 2 so multiple ticks get a chance
    // to fire. If the bug regresses, `_onWatchdogTick` reads a stamped-at-arm
    // timestamp, sees elapsed > timeout, and flips `fired`.
    QTest::qWait(1200);
    QVERIFY2(!fired, "Watchdog must not fire before any frame has been delivered");

    primary.stream->bridge()->disarmWatchdog();
    QObject::disconnect(conn);

    primary.stream->registerVideoSink(nullptr);
    processEvents();
}

// ─────────────────────────────────────────────────────────────────────────────
// Dark-corner error / lifecycle paths
// ─────────────────────────────────────────────────────────────────────────────

void VideoManagerIntegrationTest::_testReceiverFatalErrorSurfacesViaLastError()
{
    // When a receiver emits receiverError(Fatal, ...), VideoStream must
    // publish it via lastError/lastErrorCategory and fire lastErrorChanged
    // so QML can show the user a message.
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    QSignalSpy errSpy(primary.stream, &VideoStream::lastErrorChanged);

    const QString msg = QStringLiteral("decoder exploded");
    primary.fake->emitReceiverError(VideoReceiver::ErrorCategory::Fatal, msg);
    processEvents();

    QCOMPARE(errSpy.count(), 1);
    QCOMPARE(primary.stream->lastErrorCategory(), VideoReceiver::ErrorCategory::Fatal);
    QVERIFY2(primary.stream->lastError().contains(msg),
             "lastError must include the receiver-supplied message");
    QVERIFY2(primary.stream->lastError().contains(QLatin1String("[fatal]")),
             "Fatal category must be tagged in the formatted lastError string");
}

void VideoManagerIntegrationTest::_testUriClearDestroysReceiver()
{
    // Clearing a stream's URI tears down the receiver (documented path in
    // VideoStream::setUri). The bridge survives because it's owned by the
    // stream session — this is the invariant the session refactor relies on.
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);
    QVERIFY(primary.stream->receiver() != nullptr);

    VideoFrameDelivery* bridgeBefore = primary.stream->bridge();
    QVERIFY(bridgeBefore != nullptr);

    primary.stream->setUri(QString());
    processEvents();

    QCOMPARE(primary.stream->receiver(), nullptr);
    QCOMPARE(primary.stream->bridge(), bridgeBefore);  // bridge persists across receiver swap
}

void VideoManagerIntegrationTest::_testRepeatedSinkRegistrationIsIdempotent()
{
    // registerVideoSink() short-circuits when the same sink is already wired.
    // Ensures rapid/duplicate QGCVideoOutput mount paths don't fire spurious
    // bridgeChanged notifications or churn the receiver's sink wiring.
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    QVideoSink sink;
    QSignalSpy bridgeSpy(primary.stream, &VideoStream::bridgeChanged);

    primary.stream->registerVideoSink(&sink);
    processEvents();
    const int firstEmits = bridgeSpy.count();
    QVERIFY2(firstEmits >= 1, "first registration must emit bridgeChanged");

    // Idempotent: same sink again shouldn't re-emit.
    primary.stream->registerVideoSink(&sink);
    processEvents();
    QCOMPARE(bridgeSpy.count(), firstEmits);

    primary.stream->registerVideoSink(nullptr);
    processEvents();
}

void VideoManagerIntegrationTest::_testSinkDestructionClearsBridgeAutomatically()
{
    // VideoFrameDelivery::_applyVideoSink() connects the sink's destroyed()
    // signal so a consumer that deletes its QVideoSink doesn't leave a
    // dangling pointer in the bridge. Verify the auto-clear path.
    VideoManager mgr;
    auto primary = buildAndRegisterStream(mgr, VideoStream::Role::Primary);

    auto* sink = new QVideoSink;
    primary.stream->registerVideoSink(sink);
    processEvents();
    QCOMPARE(primary.stream->bridge()->videoSink(), sink);

    delete sink;  // fires QObject::destroyed → bridge clears _sink
    processEvents();

    QVERIFY2(primary.stream->bridge()->videoSink() == nullptr,
             "Bridge must auto-clear its sink pointer when the sink is destroyed");
}

// ─── #1 dynamic multi-stream topology ──────────────────────────────────────

namespace {
/// Build a QGCVideoStreamInfo from a minimal MAVLink-style struct so the test
/// doesn't need a live vehicle + camera manager to exercise the reconcile path.
QGCVideoStreamInfo* makeStreamInfo(quint8 id, const char* uri, bool thermal, QObject* parent)
{
    mavlink_video_stream_information_t info{};
    info.stream_id = id;
    info.flags = thermal ? VIDEO_STREAM_STATUS_FLAGS_THERMAL : 0;
    // qstrncpy is not available with implicit fallthrough here — strncpy keeps
    // the null terminator without importing <cstring> gymnastics.
    std::snprintf(info.uri, sizeof(info.uri), "%s", uri);
    std::snprintf(info.name, sizeof(info.name), "stream%u", id);
    return new QGCVideoStreamInfo(info, parent);
}
}  // namespace

void VideoManagerIntegrationTest::_testDynamicStreamsReconcile()
{
    VideoManager mgr;
    auto* orch = mgr.streamOrchestrator();
    QVERIFY(orch != nullptr);

    // Seed the orchestrator with Primary/Thermal so the reconcile can skip the
    // Primary/Thermal slots. Using `setCreateStreamsForTest` avoids spinning up
    // real receivers — we only need the stream objects registered in the model.
    orch->setCreateStreamsForTest([&orch]() {
        auto* primary = new VideoStream(VideoStream::Role::Primary,
                                         [](const VideoSourceResolver::VideoSource&, bool, QObject* p) -> VideoReceiver* {
                                             return new FakeVideoReceiver(p);
                                         },
                                         orch);
        orch->_streams.append(primary);
        orch->_wireStreamSignals(primary);
        orch->_streamModel->addStream(primary);

        auto* thermal = new VideoStream(VideoStream::Role::Thermal,
                                         [](const VideoSourceResolver::VideoSource&, bool, QObject* p) -> VideoReceiver* {
                                             return new FakeVideoReceiver(p);
                                         },
                                         orch);
        orch->_streams.append(thermal);
        orch->_wireStreamSignals(thermal);
        orch->_streamModel->addStream(thermal);
    });
    orch->createStreams();
    QCOMPARE(orch->streamModel()->streams().size(), 2);

    // Route Dynamic-stream receiver creation through FakeVideoReceiver so the
    // test doesn't try to open live RTSP connections.
    orch->setDynamicReceiverFactoryForTest(
        [](const VideoSourceResolver::VideoSource& /*source*/, bool /*thermal*/, QObject* parent) -> VideoReceiver* {
            return new FakeVideoReceiver(parent);
        });

    // Drive the reconcile with an explicit expected set (test seam bypasses
    // the real QGCCameraManager, which requires a full Vehicle).
    QObject infoOwner;
    auto* s1 = makeStreamInfo(1, "rtsp://device/stream1", /*thermal=*/false, &infoOwner);
    auto* s2 = makeStreamInfo(2, "rtsp://device/stream2", /*thermal=*/false, &infoOwner);

    QHash<quint8, QGCVideoStreamInfo*> expected{{1, s1}, {2, s2}};
    QVERIFY(orch->_reconcileDynamicStreams(expected));

    // Two Dynamic streams created on top of Primary + Thermal.
    QCOMPARE(orch->streamModel()->streams().size(), 4);
    QCOMPARE(orch->dynamicStreams().size(), 2);

    // Stable naming: `dynamicVideo.<id>` — used by QGCVideoOutput.streamName and
    // RecordingCoordinator's per-stream filename suffix.
    VideoStream* d1 = orch->streamModel()->stream(QStringLiteral("dynamicVideo.1"));
    VideoStream* d2 = orch->streamModel()->stream(QStringLiteral("dynamicVideo.2"));
    QVERIFY(d1 != nullptr);
    QVERIFY(d2 != nullptr);
    QCOMPARE(d1->role(), VideoStream::Role::Dynamic);
    QCOMPARE(d2->role(), VideoStream::Role::Dynamic);
    QCOMPARE(d1->videoStreamInfo(), s1);
    QCOMPARE(d2->videoStreamInfo(), s2);

    // Idempotent: second reconcile with the same set returns false.
    QVERIFY(!orch->_reconcileDynamicStreams(expected));
    QCOMPARE(orch->dynamicStreams().size(), 2);

    // Drop stream 1 — reconcile tears it down; stream 2 survives.
    QHash<quint8, QGCVideoStreamInfo*> reduced{{2, s2}};
    QVERIFY(orch->_reconcileDynamicStreams(reduced));
    processEvents();  // let the deleteLater() fire so model accounting is tight
    QCOMPARE(orch->dynamicStreams().size(), 1);
    QCOMPARE(orch->streamModel()->stream(QStringLiteral("dynamicVideo.1")), nullptr);
    QVERIFY(orch->streamModel()->stream(QStringLiteral("dynamicVideo.2")) != nullptr);

    // Clear the expected set — all Dynamic streams destroyed.
    QVERIFY(orch->_reconcileDynamicStreams({}));
    processEvents();
    QCOMPARE(orch->dynamicStreams().size(), 0);
}

UT_REGISTER_TEST(VideoManagerIntegrationTest, TestLabel::Unit)
