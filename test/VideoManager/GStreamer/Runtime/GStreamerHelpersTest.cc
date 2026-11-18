#include "GStreamerHelpersTest.h"

#include <atomic>
#include <gst/gst.h>
#include <gst/video/video-event.h>
#include <thread>
#include <vector>

#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>

#include "GStreamerHelpers.h"
#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "TeeBranchHead.h"

namespace {

/// Counts the events of @p type a pad probe sees, in the gint pointed to by the probe's data.
template <GstEventType type>
GstPadProbeReturn countEvents([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    GstEvent* const event = GST_PAD_PROBE_INFO_EVENT(info);
    if (event && (GST_EVENT_TYPE(event) == type)) {
        static_cast<std::atomic<int>*>(data)->fetch_add(1);
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn countKeyframeRequests([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    GstEvent* const event = GST_PAD_PROBE_INFO_EVENT(info);
    gboolean allHeaders = FALSE;
    if (event && gst_video_event_is_force_key_unit(event) &&
        gst_video_event_parse_upstream_force_key_unit(event, nullptr, &allHeaders, nullptr) && allHeaders) {
        static_cast<std::atomic<int>*>(data)->fetch_add(1);
    }
    return GST_PAD_PROBE_OK;
}

GstElement* parentOf(GstElement* element)
{
    return GST_ELEMENT(GST_OBJECT_PARENT(element));
}

/// Has the appsrc @p source push one buffer.
bool pushBuffer(GstElement* source)
{
    GstBuffer* const buffer = gst_buffer_new_allocate(nullptr, 16, nullptr);
    GstFlowReturn result = GST_FLOW_ERROR;
    // The signal takes a ref of its own.
    g_signal_emit_by_name(source, "push-buffer", buffer, &result);
    gst_buffer_unref(buffer);
    return result == GST_FLOW_OK;
}

// Enough children disposed of for some to meet a state change of their bin at the wrong moment: without the bin's
// state lock at least four in a thousand did.
constexpr int kDisposalsDuringStateChanges = 2000;

}  // namespace

void GStreamerHelpersTest::_testAddToBin()
{
    GstTest::TestPipeline pipeline(gst_pipeline_new("helpers"));
    QVERIFY(pipeline.get());

    GstElement* const added = GStreamer::addToBin(pipeline.get(), GStreamer::makeElement("identity", "id"), "identity");
    QVERIFY(added);
    QCOMPARE(parentOf(added), pipeline.get());
    // The bin's ref is the only one left.
    QCOMPARE(GST_OBJECT_REFCOUNT_VALUE(added), 1);
}

void GStreamerHelpersTest::_testAddToBinRejects()
{
    GstTest::TestPipeline pipeline(gst_pipeline_new("helpers"));
    QVERIFY(pipeline.get());

    expectLogMessage("Video.GStreamer.GStreamerHelpers", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("^Missing or floating element for missing$")));
    QVERIFY(!GStreamer::addToBin(pipeline.get(), nullptr, "missing"));
    verifyExpectedLogMessage();

    // A name the bin already holds.
    QVERIFY(GStreamer::addToBin(pipeline.get(), GStreamer::makeElement("identity", "same"), "first"));
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("not unique in bin")));
    expectLogMessage("Video.GStreamer.GStreamerHelpers", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^gst_bin_add\\(\\) failed for second$")));
    QVERIFY(!GStreamer::addToBin(pipeline.get(), GStreamer::makeElement("identity", "same"), "second"));
    verifyExpectedLogMessage();
}

void GStreamerHelpersTest::_testDisposeChild()
{
    // Played to its end, so no streaming thread is left inside the identity: without a flush, one parked downstream
    // (as in a PAUSED sink, on data or EOS) would keep the identity from stopping (see flushAndDisposeChild()).
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("fakesrc num-buffers=1 ! identity name=id ! fakesink sync=false")),
             qPrintable(pipeline.error()));
    QVERIFY(pipeline.play());
    const GStreamer::GstMessagePtr end = pipeline.waitForEosOrError();
    QVERIFY(end && (GST_MESSAGE_TYPE(end.get()) == GST_MESSAGE_EOS));
    const GStreamer::GstElementPtr identity = pipeline.element("id");
    QVERIFY(identity);
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(identity.get(), "sink"));
    std::atomic<int> flushes{0};
    GStreamer::PadProbe flushProbe;
    QVERIFY(
        flushProbe.add(sinkPad.get(), GST_PAD_PROBE_TYPE_EVENT_FLUSH, countEvents<GST_EVENT_FLUSH_START>, &flushes));

    GStreamer::disposeChild(identity.get());
    QVERIFY(!parentOf(identity.get()));
    GstState state = GST_STATE_VOID_PENDING;
    QCOMPARE(gst_element_get_state(identity.get(), &state, nullptr, 0), GST_STATE_CHANGE_SUCCESS);
    QCOMPARE(state, GST_STATE_NULL);
    QCOMPARE(flushes.load(), 0);

    // Without a parent, and without an element, there is nothing to do; the caller's ref stays good.
    GStreamer::disposeChild(identity.get());
    GStreamer::disposeChild(nullptr);
    QCOMPARE(QString::fromUtf8(GST_OBJECT_NAME(identity.get())), QStringLiteral("id"));
}

void GStreamerHelpersTest::_testDisposeChildOutlastsBinStateChange()
{
    GstTest::TestPipeline pipeline(gst_pipeline_new("helpers"));
    QVERIFY(pipeline.get());
    // Nobody reads the state changes this posts.
    const GStreamer::GstBusPtr bus = GStreamer::adoptBus(gst_element_get_bus(pipeline.get()));
    gst_bus_set_flushing(bus.get(), TRUE);
    QVERIFY(pipeline.play());

    // The bin changing state on another thread, as its continuation does on GStreamer's thread pool once an async
    // sink was taken out: it must not set a child it still holds to its own state after disposeChild() returned.
    std::atomic<bool> stop{false};
    std::thread stateChanges([&pipeline, &stop] {
        while (!stop.load()) {
            (void) gst_element_set_state(pipeline.get(), GST_STATE_PAUSED);
            (void) gst_element_set_state(pipeline.get(), GST_STATE_PLAYING);
        }
    });
    std::vector<GStreamer::GstElementPtr> children;
    children.reserve(kDisposalsDuringStateChanges);
    for (int i = 0; i < kDisposalsDuringStateChanges; ++i) {
        GStreamer::GstElementPtr child = GStreamer::makeElement("identity");
        (void) gst_bin_add(pipeline.bin(), child.get());
        (void) gst_element_sync_state_with_parent(child.get());
        GStreamer::disposeChild(child.get());
        children.push_back(std::move(child));
    }
    stop.store(true);
    stateChanges.join();

    int broughtBackUp = 0;
    for (const GStreamer::GstElementPtr& child : children) {
        if (GST_STATE(child.get()) != GST_STATE_NULL) {
            ++broughtBackUp;
            (void) gst_element_set_state(child.get(), GST_STATE_NULL);
        }
    }
    QCOMPARE(broughtBackUp, 0);
}

void GStreamerHelpersTest::_testFlushAndDisposeChildFlushes()
{
    // Once prerolled, the streaming thread waits inside the sink, holding its stream lock: the flush wakes it, so the
    // sink can stop. Without one, going NULL would wait for that thread forever.
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("fakesrc num-buffers=1 ! fakesink name=sink")),
             qPrintable(pipeline.error()));
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);
    QCOMPARE(gst_element_get_state(pipeline.get(), nullptr, nullptr, GstTest::clockTime(TestTimeout::mediumDuration())),
             GST_STATE_CHANGE_SUCCESS);
    const GStreamer::GstElementPtr sink = pipeline.element("sink");
    QVERIFY(sink);
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(sink.get(), "sink"));
    std::atomic<int> flushes{0};
    GStreamer::PadProbe flushProbe;
    QVERIFY(
        flushProbe.add(sinkPad.get(), GST_PAD_PROBE_TYPE_EVENT_FLUSH, countEvents<GST_EVENT_FLUSH_START>, &flushes));

    GStreamer::flushAndDisposeChild(sink.get());
    QCOMPARE(flushes.load(), 1);
    QVERIFY(!parentOf(sink.get()));
    GstState state = GST_STATE_VOID_PENDING;
    QCOMPARE(gst_element_get_state(sink.get(), &state, nullptr, 0), GST_STATE_CHANGE_SUCCESS);
    QCOMPARE(state, GST_STATE_NULL);
}

void GStreamerHelpersTest::_testTeeBranchHead()
{
    GstTest::TestPipeline pipeline(gst_pipeline_new("helpers"));
    QVERIFY(pipeline.get());
    GstElement* const tee = GStreamer::addToBin(pipeline.get(), GStreamer::makeElement("tee", "tee"), "tee");
    QVERIFY(tee);

    TeeBranchHead head;
    QVERIFY(head.build(pipeline.get(), tee, GStreamer::PipelineBranch::Recording, "test"));
    GstElement* const queue = head.queue();
    GstElement* const valve = head.valve();
    QVERIFY(queue);
    QVERIFY(valve);
    QCOMPARE(parentOf(queue), pipeline.get());
    QCOMPARE(parentOf(valve), pipeline.get());
    QVERIFY(head.contains(GST_OBJECT(queue)));
    QVERIFY(head.contains(GST_OBJECT(valve)));
    QVERIFY(!head.contains(GST_OBJECT(tee)));
    QCOMPARE(GStreamer::branchOf(GST_OBJECT(queue)), GStreamer::PipelineBranch::Recording);
    QCOMPARE(GStreamer::branchOf(GST_OBJECT(valve)), GStreamer::PipelineBranch::Recording);
    gboolean drop = FALSE;
    g_object_get(valve, "drop", &drop, nullptr);
    QVERIFY(drop);

    // tee ! queue ! valve
    const GStreamer::GstPadPtr queueSink = GStreamer::adoptPad(gst_element_get_static_pad(queue, "sink"));
    const GStreamer::GstPadPtr teeSrc = GStreamer::adoptPad(gst_pad_get_peer(queueSink.get()));
    QVERIFY(teeSrc);
    QCOMPARE(GST_PAD_PARENT(teeSrc.get()), tee);
    const GStreamer::GstPadPtr valveSink = GStreamer::adoptPad(gst_element_get_static_pad(valve, "sink"));
    const GStreamer::GstPadPtr queueSrc = GStreamer::adoptPad(gst_pad_get_peer(valveSink.get()));
    QVERIFY(queueSrc);
    QCOMPARE(GST_PAD_PARENT(queueSrc.get()), queue);

    // Opening asks upstream for a keyframe to start from; the pads must be active for the request to leave the valve.
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);
    std::atomic<int> requests{0};
    GStreamer::PadProbe probe;
    QVERIFY(probe.add(queueSrc.get(), GST_PAD_PROBE_TYPE_EVENT_UPSTREAM, countKeyframeRequests, &requests));
    head.open();
    g_object_get(valve, "drop", &drop, nullptr);
    QVERIFY(!drop);
    QCOMPARE(requests.load(), 1);

    head.close();
    g_object_get(valve, "drop", &drop, nullptr);
    QVERIFY(drop);

    head.reset();
    QVERIFY(!head.queue());
    QVERIFY(!head.valve());
    head.open();
    head.close();
    QCOMPARE(requests.load(), 1);
}

void GStreamerHelpersTest::_testTeeBranchHeadOpensBehindPushInFlight()
{
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc");
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("appsrc name=src ! tee name=tee")), qPrintable(pipeline.error()));
    const GStreamer::GstElementPtr source = pipeline.element("src");
    const GStreamer::GstElementPtr tee = pipeline.element("tee");
    TeeBranchHead head;
    QVERIFY(head.build(pipeline.get(), tee.get(), GStreamer::PipelineBranch::Decoding, "test"));
    GstElement* const first =
        GStreamer::addToBin(pipeline.get(), GStreamer::makeElement("fakesink", "first"), "fakesink");
    QVERIFY(first && gst_element_link(head.valve(), first));
    QVERIFY(pipeline.play());
    head.open();

    // A push is under way in what follows the valve...
    const GStreamer::GstPadPtr firstSink = GStreamer::adoptPad(gst_element_get_static_pad(first, "sink"));
    std::atomic<int> held{0};
    GStreamer::PadProbe hold;
    QVERIFY(hold.add(firstSink.get(),
                     static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_BUFFER),
                     GstTest::countBuffers, &held));
    QVERIFY(pushBuffer(source.get()));
    QTRY_COMPARE_WITH_TIMEOUT(held.load(), 1, TestTimeout::mediumMs());

    // ... while the branch replaces that part by another.
    head.close();
    gst_element_unlink(head.valve(), first);
    GstElement* const second =
        GStreamer::addToBin(pipeline.get(), GStreamer::makeElement("fakesink", "second"), "fakesink");
    QVERIFY(second && gst_element_sync_state_with_parent(second) && gst_element_link(head.valve(), second));
    const GStreamer::GstPadPtr secondSink = GStreamer::adoptPad(gst_element_get_static_pad(second, "sink"));
    std::atomic<int> arrived{0};
    GStreamer::PadProbe arrival;
    QVERIFY(arrival.add(secondSink.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &arrived));

    // The valve stays closed until that push is back: it comes back flushing, as from a part that was stopped, which
    // an open valve would hand to the queue, whose streaming thread would stop for it.
    std::atomic<bool> opening{false};
    std::thread opener([&head, &opening] {
        opening.store(true);
        head.open();
    });
    QTRY_VERIFY_WITH_TIMEOUT(opening.load(), TestTimeout::mediumMs());
    gboolean drop = FALSE;
    g_object_get(head.valve(), "drop", &drop, nullptr);
    const bool closedDuringPush = drop;
    (void) gst_pad_send_event(firstSink.get(), gst_event_new_flush_start());
    opener.join();
    QVERIFY(closedDuringPush);

    // The queue still passes on what follows.
    QVERIFY(pushBuffer(source.get()));
    QTRY_COMPARE_WITH_TIMEOUT(arrived.load(), 1, TestTimeout::mediumMs());
}

void GStreamerHelpersTest::_testRequestKeyframe()
{
    GStreamer::requestKeyframe(nullptr);

    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("fakesrc name=src num-buffers=1 ! fakesink name=sink")),
             qPrintable(pipeline.error()));
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);
    const GStreamer::GstPadPtr srcPad =
        GStreamer::adoptPad(gst_element_get_static_pad(pipeline.element("src").get(), "src"));
    std::atomic<int> requests{0};
    GStreamer::PadProbe probe;
    QVERIFY(probe.add(srcPad.get(), GST_PAD_PROBE_TYPE_EVENT_UPSTREAM, countKeyframeRequests, &requests));

    // An upstream force-key-unit event asking for the parameter sets too, which reaches the source at once.
    const GStreamer::GstPadPtr sinkPad =
        GStreamer::adoptPad(gst_element_get_static_pad(pipeline.element("sink").get(), "sink"));
    GStreamer::requestKeyframe(sinkPad.get());
    QCOMPARE(requests.load(), 1);
}

void GStreamerHelpersTest::_testSendEventAsync()
{
    // A pad without an element has no thread pool to send from: the event is dropped.
    const GStreamer::GstPadPtr orphan = GStreamer::adoptFloating(gst_pad_new("orphan", GST_PAD_SINK));
    GStreamer::sendEventAsync(orphan.get(), gst_event_new_eos());
    GStreamer::sendEventAsync(nullptr, gst_event_new_eos());

    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("fakesrc num-buffers=1 ! identity name=id ! fakesink")),
             qPrintable(pipeline.error()));
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);
    const GStreamer::GstPadPtr sinkPad =
        GStreamer::adoptPad(gst_element_get_static_pad(pipeline.element("id").get(), "sink"));
    std::atomic<int> custom{0};
    GStreamer::PadProbe probe;
    QVERIFY(probe.add(sinkPad.get(), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, countEvents<GST_EVENT_CUSTOM_DOWNSTREAM_OOB>,
                      &custom));

    GStreamer::sendEventAsync(
        sinkPad.get(), gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, gst_structure_new_empty("qgc-test")));
    QTRY_COMPARE_WITH_TIMEOUT(custom.load(), 1, TestTimeout::mediumMs());
}

void GStreamerHelpersTest::_testFirstSrcPad()
{
    const GStreamer::GstElementPtr sink = GStreamer::makeElement("fakesink");
    QVERIFY(sink);
    QVERIFY(!GStreamer::firstSrcPad(sink.get()));

    const GStreamer::GstElementPtr source = GStreamer::makeElement("fakesrc");
    QVERIFY(source);
    const GStreamer::GstPadPtr pad = GStreamer::firstSrcPad(source.get());
    QVERIFY(pad);
    QCOMPARE(QString::fromUtf8(GST_PAD_NAME(pad.get())), QStringLiteral("src"));
    QCOMPARE(GST_PAD_PARENT(pad.get()), source.get());
}

void GStreamerHelpersTest::_testHoldPadUntilLinked()
{
    const GStreamer::GstPadPtr pad = GStreamer::adoptFloating(gst_pad_new("src", GST_PAD_SRC));
    GstPad* linked = nullptr;
    const std::function<void()> link =
        GStreamer::holdPadUntilLinked(pad.get(), [&linked](GstPad* held) { linked = held; });
    QVERIFY(gst_pad_is_blocked(pad.get()));
    QVERIFY(!linked);

    link();
    QCOMPARE(linked, pad.get());
    QVERIFY(!gst_pad_is_blocked(pad.get()));
}

void GStreamerHelpersTest::_testForEachPlugin()
{
    int visits = 0;
    GStreamer::forEachPlugin(nullptr, [&visits](GstPlugin*) { ++visits; });
    GStreamer::forEachPlugin(gst_registry_get(), {});
    QCOMPARE(visits, 0);

    QStringList names;
    GStreamer::forEachPlugin(gst_registry_get(), [&names](GstPlugin* plugin) {
        names.append(QString::fromUtf8(gst_plugin_get_name(plugin)));
    });
    QVERIFY(names.contains(QStringLiteral("coreelements")));
}

UT_REGISTER_TEST(GStreamerHelpersTest, TestLabel::Integration)
