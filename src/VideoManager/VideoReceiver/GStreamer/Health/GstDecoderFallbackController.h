#pragma once

#include <QtCore/QString>
#include <gst/gstmessage.h>

class GstDecodingBranch;

/// Encapsulates the HW→SW decoder demotion decision.
///
/// GstVideoReceiver::_tryDecoderFallback delegates to this class to keep
/// the policy (check, mark, log) separate from the GStreamer side-effects
/// (stop/start scheduling) that remain in the caller. Fallback is scoped
/// per-branch via `force-sw-decoders` on the rebuilt decodebin — no global
/// GStreamer registry mutation.
///
/// All methods must be called from the GStreamer worker thread.
class GstDecoderFallbackController
{
public:
    GstDecoderFallbackController() = default;

    struct FallbackResult
    {
        bool shouldRetry = false;
        QString decoderName;  ///< plugin feature name of the failing decoder
    };

    /// Inspect \a errorMsg and \a branch to decide whether a SW fallback
    /// should be attempted.  On a positive decision the branch's failed flag
    /// is set (via markFailed()) before returning.
    FallbackResult evaluate(GstMessage* errorMsg, GstDecodingBranch& branch);

    /// Mark the branch as having experienced a HW decoder failure.
    /// Delegates to branch.setHwDecoderFailed(true).
    void markFailed(GstDecodingBranch& branch);
};
