#include "GStreamerDecoderRankPlan.h"

#include <algorithm>
#include <gst/gstpluginfeature.h>

namespace GStreamer {

namespace {

using enum VideoDecoder::Options;

// What forcing a decoder class or family gives it. GStreamer already ranks some decoders PRIMARY + 1 (the d3d11 and va
// ones), so a forced family goes above the highest other decoder (see prioritizeFamily).
constexpr uint16_t kPrioritizedRank = GST_RANK_PRIMARY + 1;
// Above kPrioritizedRank, so the GPU-memory decoders the platform prefers win autoplug.
constexpr uint16_t kZeroCopyRank = GST_RANK_PRIMARY + 2;

class Planner
{
public:
    explicit Planner(const QList<DecoderInfo>& decoders)
        : _decoders(decoders)
        , _ranks(decoders.size())
    {}

    template <typename Predicate>
    qsizetype rank(Predicate&& matches, uint16_t rank)
    {
        qsizetype ranked = 0;
        for (qsizetype i = 0; i < _decoders.size(); ++i) {
            if (matches(_decoders.at(i))) {
                _ranks[i] = rank;
                ++ranked;
            }
        }
        return ranked;
    }

    void rankPrefix(const char* prefix, uint16_t rank)
    {
        (void) this->rank([prefix](const DecoderInfo& decoder) { return decoder.name.startsWith(prefix); }, rank);
    }

    void rankFamily(VideoDecoder::Options family, uint16_t rank)
    {
        (void) this->rank([family](const DecoderInfo& decoder) { return decoder.family == family; }, rank);
    }

    // @p family's decoders, at least kPrioritizedRank, above every other decoder; those stay as fallbacks for the
    // codecs the family can't decode.
    void prioritizeFamily(VideoDecoder::Options family)
    {
        unsigned highestOtherRank = GST_RANK_NONE;
        for (const DecoderInfo& decoder : _decoders) {
            if (decoder.family != family) {
                highestOtherRank = std::max(highestOtherRank, decoder.rank);
            }
        }
        const unsigned rank = std::max<unsigned>(kPrioritizedRank, std::min<unsigned>(highestOtherRank, 0xFFFE) + 1);
        rankFamily(family, static_cast<uint16_t>(rank));
    }

    // Every decoder of the class at kPrioritizedRank, and every autoplugged one of the other class out of autoplug.
    void prioritizeClass(bool hardware, QStringList& warnings)
    {
        const qsizetype matched =
            rank([hardware](const DecoderInfo& decoder) { return decoder.hardware == hardware; }, kPrioritizedRank);
        if (!_decoders.isEmpty() && (matched == 0)) {
            warnings.append(QStringLiteral("No %1 video decoder factories found to reprioritize.")
                                .arg(hardware ? QStringLiteral("hardware") : QStringLiteral("software")));
        }
        (void) rank(
            [hardware](const DecoderInfo& decoder) {
                return (decoder.hardware != hardware) && (decoder.rank >= GST_RANK_MARGINAL);
            },
            GST_RANK_NONE);
    }

    QList<RankChange> changes() const
    {
        QList<RankChange> changes;
        for (qsizetype i = 0; i < _decoders.size(); ++i) {
            if (_ranks.at(i)) {
                changes.append({.feature = _decoders.at(i).name, .rank = *_ranks.at(i)});
            }
        }
        return changes;
    }

private:
    const QList<DecoderInfo>& _decoders;
    QList<std::optional<uint16_t>> _ranks;
};

}  // namespace

RankPlan planRanks(VideoDecoder::Options option, const QList<DecoderInfo>& decoders, const RankTraits& traits)
{
    RankPlan plan;
    Planner planner(decoders);

    switch (option) {
        case ForceVideoDecoderDefault:
            if (traits.preferZeroCopyDecoders) {
                planner.rankPrefix("vaapi", GST_RANK_NONE);
                planner.rankFamily(ForceVideoDecoderVAAPI, kZeroCopyRank);
                planner.rankPrefix("v4l2sl", kZeroCopyRank);
            }
            break;
        case ForceVideoDecoderSoftware:
            planner.prioritizeClass(false, plan.warnings);
            break;
        case ForceVideoDecoderHardware:
            planner.prioritizeClass(true, plan.warnings);
            break;
        case ForceVideoDecoderVulkan:
            // gst-vulkan decodes on its own VkDevice, which fails the GPU path's device-match guard: CPU copy.
            plan.warnings.append(
                QStringLiteral("Forcing Vulkan video decoders: zero-copy import is dormant "
                               "(foreign VkDevice → CPU fallback), so decode will not be zero-copy."));
            [[fallthrough]];
        case ForceVideoDecoderVAAPI:
        case ForceVideoDecoderNVIDIA:
        case ForceVideoDecoderDirectX3D:
        case ForceVideoDecoderVideoToolbox:
        case ForceVideoDecoderIntel:
            planner.prioritizeFamily(option);
            break;
    }

    if (traits.rhiD3DApi) {
        const bool d3d12 = (*traits.rhiD3DApi == D3DApi::D3D12);
        // Forcing the D3D family already ranks it above every other decoder, so only the other API is demoted then.
        if ((option == ForceVideoDecoderDefault) || (option == ForceVideoDecoderHardware)) {
            planner.rankPrefix(d3d12 ? "d3d12" : "d3d11", kZeroCopyRank);
        }
        planner.rankPrefix(d3d12 ? "d3d11" : "d3d12", GST_RANK_NONE);
    }

    plan.changes = planner.changes();
    return plan;
}

}  // namespace GStreamer
