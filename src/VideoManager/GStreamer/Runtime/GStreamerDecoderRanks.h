#pragma once

#include <cstdint>
#include <functional>
#include <gst/gst.h>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QString>

#include "GStreamerDecoderRankPlan.h"
#include "VideoDecoderOptions.h"

// Video decoder classification and the autoplug rank overrides behind the forceVideoDecoder setting. Every rank change
// goes through here, so a feature named in GST_PLUGIN_FEATURE_RANK keeps the user's rank.
namespace GStreamer {

/// Invokes @p visitor for every video decoder factory of at least @p minRank in the default registry. The factory is
/// valid only for the duration of the call.
void forEachVideoDecoderFactory(GstRank minRank, const std::function<void(GstElementFactory*)>& visitor);

bool isHardwareDecoderFactory(GstElementFactory* factory);

/// "[HW|SW] <plugin>/<factory> rank=<rank> (<klass>)" for decoder logs.
QString describeDecoderFactory(GstElementFactory* factory);

/// The decoder family that claims decoder factory @p factoryName by name prefix, or VideoDecoderOption::Default when
/// it belongs to no user-selectable hardware family.
VideoDecoderOption decoderFamilyForFactory(const char* factoryName);

/// Hardware decoder families currently present in the GStreamer registry (never VideoDecoderOption::Default or
/// Software). Lets the settings layer validate a persisted forceVideoDecoder choice against what the running build can
/// actually provide. Vulkan is only offered with the opt-in Vulkan GPU path.
QList<VideoDecoderOption> availableDecoderFamilies();

/// Applies decoder rank plans (planRanks()) to the default registry. It remembers the rank each feature had before QGC
/// first changed it, so a new option starts over from those rather than adding to the previous one. A feature named in
/// GST_PLUGIN_FEATURE_RANK keeps the user's rank. One per process, as the registry is. GUI thread, except
/// demoteFailedDecoder().
class DecoderRankApplier
{
public:
    static DecoderRankApplier& instance();

    DecoderRankApplier(const DecoderRankApplier&) = delete;
    DecoderRankApplier& operator=(const DecoderRankApplier&) = delete;

    /// Restores the original ranks, then applies @p option's plan for the installed decoders on this platform.
    void apply(VideoDecoderOption option);
    /// apply() for the option applied last, when the platform's RankTraits changed since: Qt's device became known.
    void reapply();
    /// Restores the original ranks.
    void restore();

    /// The option apply() last applied; nullopt before the first and after restore().
    std::optional<VideoDecoderOption> applied() const { return _applied; }

    /// Sets @p featureName's rank in @p registry. False when the registry or name is null or the feature is absent.
    bool setRank(GstRegistry* registry, const char* featureName, uint16_t rank);

    /// Takes the decoder @p featureName, which failed on a stream with @p caps, out of autoplug until the next apply()
    /// or restore(), so that the next decodebin3 plugs the decoder ranked after it. False, changing nothing, when no
    /// other autoplugged decoder takes @p caps or GST_PLUGIN_FEATURE_RANK names the decoder. Any thread.
    bool demoteFailedDecoder(const char* featureName, const GstCaps* caps);

private:
    DecoderRankApplier() = default;

    static GstRegistry* _registry();
    /// setRank(), with _mutex held.
    bool _setRank(GstRegistry* registry, const char* featureName, uint16_t rank);
    /// With _mutex held.
    void _restore(GstRegistry* registry);

    /// Guards _originalRanks, which a receiver's worker changes too (demoteFailedDecoder()).
    QMutex _mutex;
    QHash<QByteArray, guint> _originalRanks;
    std::optional<VideoDecoderOption> _applied;
    RankTraits _appliedTraits;
};

/// Applies @p option for its lifetime, then re-applies the option applied before it, or the original ranks when none
/// was. GUI thread.
class ScopedDecoderOption
{
public:
    explicit ScopedDecoderOption(VideoDecoderOption option);
    ~ScopedDecoderOption();

    ScopedDecoderOption(const ScopedDecoderOption&) = delete;
    ScopedDecoderOption& operator=(const ScopedDecoderOption&) = delete;

private:
    const std::optional<VideoDecoderOption> _previous;
};

/// DecoderRankApplier::setRank() on the process's applier, so setCodecPriorities() restores the rank too.
bool changeFeatureRank(GstRegistry* registry, const char* featureName, uint16_t rank);

/// DecoderRankApplier::apply() on the process's applier. GUI thread.
void setCodecPriorities(VideoDecoderOption option);

/// DecoderRankApplier::demoteFailedDecoder() on the process's applier. Any thread.
bool demoteFailedDecoder(const char* factoryName, const GstCaps* caps);

// Diagnostic dump of installed video-decoder factories, sorted by rank, with HW/SW classification, at debug level:
// does nothing while the Video.GStreamer.DecoderRanks category is off.
void logDecoderRanks();

}  // namespace GStreamer
