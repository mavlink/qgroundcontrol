#pragma once

#include <cstdint>
#include <functional>
#include <gst/gst.h>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GStreamer.h"  // VideoDecoderOptions
#include "GStreamerDecoderRankPlan.h"

// Video decoder classification and the autoplug rank overrides behind the forceVideoDecoder setting. Every rank change
// goes through here, so a feature named in GST_PLUGIN_FEATURE_RANK keeps the user's rank.
namespace GStreamer {

/// Invokes @p visitor for every video decoder factory of at least @p minRank in the default registry. The factory is
/// valid only for the duration of the call.
void forEachVideoDecoderFactory(GstRank minRank, const std::function<void(GstElementFactory*)>& visitor);

bool isHardwareDecoderFactory(GstElementFactory* factory);

/// "[HW|SW] <plugin>/<factory> rank=<rank> (<klass>)" for decoder logs.
QString describeDecoderFactory(GstElementFactory* factory);

/// ForceVideoDecoder* family that claims decoder factory @p factoryName by name prefix, or
/// ForceVideoDecoderDefault when it belongs to no user-selectable hardware family.
VideoDecoderOptions decoderFamilyForFactory(const char* factoryName);

/// Hardware decoder families currently present in the GStreamer registry, as VideoDecoderOptions
/// values (always omits ForceVideoDecoderDefault/Software). Lets the settings layer validate a
/// persisted ForceVideoDecoder* choice against what the running build can actually provide. Vulkan is
/// only offered with the opt-in Vulkan GPU path.
QList<VideoDecoderOptions> availableDecoderFamilies();

/// The RankTraits of the running platform: the zero-copy preferences on Linux, the RHI's D3D API on Windows.
RankTraits currentRankTraits();

/// Applies decoder rank plans (planRanks()) to the default registry. It remembers the rank each feature had before QGC
/// first changed it, so a new option starts over from those rather than adding to the previous one. A feature named in
/// GST_PLUGIN_FEATURE_RANK keeps the user's rank. One per process, as the registry is. GUI thread.
class DecoderRankApplier
{
public:
    static DecoderRankApplier& instance();

    /// Restores the original ranks, then applies @p option's plan for the installed decoders and currentRankTraits().
    void apply(VideoDecoderOptions option);
    /// Restores the original ranks.
    void restore();

    /// The option apply() last applied; nullopt before the first and after restore().
    std::optional<VideoDecoderOptions> applied() const { return _applied; }

    /// Sets @p featureName's rank in @p registry. False when the registry or name is null or the feature is absent.
    bool setRank(GstRegistry* registry, const char* featureName, uint16_t rank);

private:
    DecoderRankApplier() = default;

    static GstRegistry* _registry();
    void _restore(GstRegistry* registry);

    QHash<QByteArray, guint> _originalRanks;
    std::optional<VideoDecoderOptions> _applied;
};

/// Applies @p option for its lifetime, then re-applies the option applied before it, or the original ranks when none
/// was. GUI thread.
class ScopedDecoderOption
{
public:
    explicit ScopedDecoderOption(VideoDecoderOptions option);
    ~ScopedDecoderOption();

    ScopedDecoderOption(const ScopedDecoderOption&) = delete;
    ScopedDecoderOption& operator=(const ScopedDecoderOption&) = delete;

private:
    const std::optional<VideoDecoderOptions> _previous;
};

/// DecoderRankApplier::setRank() on the process's applier, so setCodecPriorities() restores the rank too.
bool changeFeatureRank(GstRegistry* registry, const char* featureName, uint16_t rank);

/// DecoderRankApplier::apply() on the process's applier. GUI thread.
void setCodecPriorities(VideoDecoderOptions option);
/// Overload taking the raw forceVideoDecoder setting value; the cast/range-check lives in the impl.
void setCodecPriorities(int rawOption);

// Diagnostic dump of installed video-decoder factories, sorted by rank, with
// HW/SW classification. No-op once the registry is empty.
void logDecoderRanks();

}  // namespace GStreamer
