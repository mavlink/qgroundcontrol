// GstVideoReceiver static callbacks and probe handlers.

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <utility>

#include "GStreamerHelpers.h"
#include "GstCallbackDispatch.h"
#include "GstObjectPtr.h"
#include "GstVideoReceiver.h"
#include "StreamHealthMonitor.h"

gboolean GstVideoReceiver::_onBusMessage(GstBus* /* bus */, GstMessage* msg, gpointer data)
{
    if (!msg || !data) {
        qCCritical(GstVideoReceiverLog) << "Invalid parameters in _onBusMessage: msg=" << msg << "data=" << data;
        return TRUE;
    }

    GstVideoReceiver* pThis = self(data);

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            gchar* debug;
            GError* error;
            gst_message_parse_error(msg, &error, &debug);

            QString errorMessage;
            if (debug) {
                qCDebug(GstVideoReceiverLog) << "GStreamer debug:" << debug;
                g_clear_pointer(&debug, g_free);
            }

            if (error) {
                qCCritical(GstVideoReceiverLog) << "GStreamer error:" << error->message;
                errorMessage = QString::fromUtf8(error->message);
                g_clear_error(&error);
            }

            // Take an owning ref on msg so it outlives this sync-message handler;
            // GstMessagePtr releases it whether the lambda runs or is skipped.
            GstMessagePtr msgRef(gst_message_ref(msg));
            GstCallbackDispatch::dispatchGuarded(
                pThis, pThis->_destroyed,
                [pThis, msg = std::move(msgRef), errorMessage]() {
                    if (pThis->_tryDecoderFallback(msg.get())) {
                        emit pThis->receiverError(
                            VideoReceiver::ErrorCategory::HardwareFallback,
                            QStringLiteral("HW decoder failed — retrying with software path: %1").arg(errorMessage));
                    } else {
                        qCDebug(GstVideoReceiverLog) << "Stopping because of error";
                        emit pThis->receiverError(VideoReceiver::ErrorCategory::Fatal, errorMessage);
                        pThis->stop();
                    }
                });
            break;
        }
        case GST_MESSAGE_EOS: {
            // Pipeline-level EOS -- set flag before dispatching so _handleEOS sees it.
            // Release fence pairs with the acquire load in _handleEOS to guarantee
            // visibility without cross-thread data races.
            pThis->_endOfStream.store(true, std::memory_order_release);
            GstCallbackDispatch::dispatchGuarded(pThis, pThis->_destroyed, [pThis]() {
                qCDebug(GstVideoReceiverLog) << "Received pipeline EOS";
                pThis->_handleEOS();
            });
            break;
        }
        case GST_MESSAGE_LATENCY: {
            // A pipeline element changed its latency — recalculate so the
            // clock compensates correctly (important after RTSP UDP→TCP fallback
            // or decoder changes that affect buffering depth).
            GstCallbackDispatch::dispatchGuarded(pThis, pThis->_destroyed, [pThis]() {
                if (pThis->_pipelineController.hasPipeline()) {
                    (void)gst_bin_recalculate_latency(GST_BIN(pThis->_pipelineController.pipeline()));
                }
            });
            break;
        }
        case GST_MESSAGE_ELEMENT: {
            // Detect missing-plugin messages posted by decodebin/parsebin when a
            // codec decoder or demuxer isn't installed. Surface a clear operator
            // diagnostic rather than letting the subsequent generic ERROR bury it.
            if (gst_is_missing_plugin_message(msg)) {
                gchar* desc = gst_missing_plugin_message_get_description(msg);
                gchar* installer = gst_missing_plugin_message_get_installer_detail(msg);
                const QString descStr = QString::fromUtf8(desc ? desc : "(unknown)");
                qCCritical(GstVideoReceiverLog) << "Missing GStreamer plugin:" << descStr
                                                << "— install detail:" << (installer ? installer : "(none)");
                emit pThis->receiverError(VideoReceiver::ErrorCategory::MissingPlugin,
                                          QStringLiteral("Missing GStreamer plugin: %1").arg(descStr));
                g_free(desc);
                g_free(installer);
                break;
            }

            const GstStructure* structure = gst_message_get_structure(msg);
            if (!gst_structure_has_name(structure, "GstBinForwarded")) {
                break;
            }

            GstMessage* forward_msg = nullptr;
            gst_structure_get(structure, "message", GST_TYPE_MESSAGE, &forward_msg, NULL);
            if (!forward_msg) {
                break;
            }

            if (GST_MESSAGE_TYPE(forward_msg) == GST_MESSAGE_EOS) {
                GstCallbackDispatch::dispatchGuarded(pThis, pThis->_destroyed, [pThis]() {
                    qCDebug(GstVideoReceiverLog) << "Received branch EOS";
                    pThis->_handleEOS();
                });
            }

            gst_clear_message(&forward_msg);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

void GstVideoReceiver::_onNewPad(GstElement* element, GstPad* pad, gpointer data)
{
    // pad-added fires on the streaming thread — dispatch to the worker thread
    // so _onNewSourcePad/_onNewDecoderPad don't race with stop()/teardown.
    // Ref the pad and element so they outlive the lambda; RAII wrappers release
    // them on the worker after the lambda runs (or if the dispatch is skipped).
    GstVideoReceiver* pThis = self(data);

    GstNonFloatingPtr<GstPad> padRef(GST_PAD(gst_object_ref(pad)));
    GstNonFloatingPtr<GstElement> elemRef(GST_ELEMENT(gst_object_ref(element)));

    // Synchronously link the source's new video pad to the tee on the streaming
    // thread, BEFORE dispatching to the worker. rtspsrc starts pushing RTP/RTCP
    // buffers immediately after emitting pad-added; if our link is deferred via
    // the worker-thread dispatch below, udpsrc1 (RTCP) fires "streaming stopped,
    // reason not-linked" because its output pad has no peer. That flow error
    // cascades into rtpjitterbuffer and stalls the whole pipeline, manifesting
    // as the intermittent "Source timeout: no frames for 9 s" watchdog on cold
    // boot. GStreamer's element/pad link API is thread-safe.
    if (elemRef.get() == pThis->_pipelineController.source() && pThis->_pipelineController.tee()) {
        GstCapsPtr padCaps(gst_pad_query_caps(pad, nullptr));
        bool isVideo = false;
        if (padCaps) {
            for (guint i = 0; i < gst_caps_get_size(padCaps.get()); ++i) {
                const gchar* name = gst_structure_get_name(gst_caps_get_structure(padCaps.get(), i));
                if (name && (g_str_has_prefix(name, "video/") || g_str_has_prefix(name, "image/"))) {
                    isVideo = true;
                    break;
                }
            }
        } else {
            // No caps yet — assume video (rtspsrc may not have caps set at pad-added
            // time). The worker-thread follow-up will re-check and bail if wrong.
            isVideo = true;
        }
        if (isVideo) {
            GstNonFloatingPtr<GstPad> teeSink(gst_element_get_static_pad(pThis->_pipelineController.tee(), "sink"));
            if (teeSink && !gst_pad_is_linked(teeSink.get())) {
                const GstPadLinkReturn ret = gst_pad_link(pad, teeSink.get());
                if (ret != GST_PAD_LINK_OK) {
                    qCWarning(GstVideoReceiverLog)
                        << "Early pad link source→tee failed:" << ret << "— will retry on worker thread";
                }
            }
        }
    }

    GstCallbackDispatch::dispatchGuarded(
        pThis, pThis->_destroyed,
        [pThis, padRef = std::move(padRef), elemRef = std::move(elemRef)]() {
            if (elemRef.get() == pThis->_pipelineController.source()) {
                pThis->_onNewSourcePad(padRef.get());
            } else if (elemRef.get() == pThis->_decodingBranch.decoder()) {
                pThis->_onNewDecoderPad(padRef.get());
            } else {
                qCDebug(GstVideoReceiverLog) << "pad-added on unrecognized element (stale after teardown?)";
            }
        });
}

GstPadProbeReturn GstVideoReceiver::_teeProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad);
    Q_UNUSED(info)

    if (user_data) {
        self(user_data)->_noteTeeFrame();
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn GstVideoReceiver::_videoSinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    Q_UNUSED(pad);
    Q_UNUSED(info);

    // Notes each buffer into StreamHealthMonitor's video-sink watchdog and
    // flips _decoderActive to true on the first buffer post-start. The old
    // appsink architecture's sink-swap / MPEG-TS segment-reset workaround
    // is gone — the appsink is a permanent pipeline element, so there is
    // no live sink replacement that would strand segment info downstream.
    if (user_data)
        self(user_data)->_noteVideoSinkFrame();

    return GST_PAD_PROBE_OK;
}

void GstVideoReceiver::_noteTeeFrame()
{
    // Called on the GStreamer streaming thread. StreamHealthMonitor uses an
    // atomic timestamp internally so this is safe without a dispatch hop.
    if (_healthMonitor) {
        _healthMonitor->noteSourceFrame();
    }
}

void GstVideoReceiver::_noteVideoSinkFrame()
{
    _decodingBranch.noteVideoSinkFrame();
    if (!_decoderActive) {
        qCDebug(GstVideoReceiverLog) << "Decoding started";
        _setDecoderActive(true);
    }
}

// GstJitterTuning::tune definition now lives in StreamHealthMonitor.cc.
