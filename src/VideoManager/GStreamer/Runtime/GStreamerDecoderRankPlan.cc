#include "GStreamerDecoderRankPlan.h"

#include <algorithm>
#include <gst/gstpluginfeature.h>

namespace GStreamer {

namespace {

// What forcing a decoder family gives it at least. GStreamer already ranks some decoders PRIMARY + 1 (the d3d11 and va
// ones), so a forced family goes above the highest other decoder (see prioritizeFamily).
constexpr uint16_t kPrioritizedRank = GST_RANK_PRIMARY + 1;
// Above kPrioritizedRank, so the GPU-memory decoders the platform prefers win autoplug.
constexpr uint16_t kZeroCopyRank = GST_RANK_PRIMARY + 2;

bool autoplugged(const DecoderInfo& decoder)
{
    return decoder.rank >= GST_RANK_MARGINAL;
}

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

    void rankFamily(VideoDecoderOption family, uint16_t rank)
    {
        (void) this->rank([family](const DecoderInfo& decoder) { return decoder.family == family; }, rank);
    }

    // @p family's decoders, at least kPrioritizedRank, above every other decoder; those stay as fallbacks for the
    // codecs the family can't decode. Returns the rank the family got.
    uint16_t prioritizeFamily(VideoDecoderOption family)
    {
        unsigned highestOtherRank = GST_RANK_NONE;
        for (const DecoderInfo& decoder : _decoders) {
            if (decoder.family != family) {
                highestOtherRank = std::max(highestOtherRank, decoder.rank);
            }
        }
        const unsigned rank = std::max<unsigned>(kPrioritizedRank, std::min<unsigned>(highestOtherRank, 0xFFFD) + 1);
        rankFamily(family, static_cast<uint16_t>(rank));
        return static_cast<uint16_t>(rank);
    }

    // Every autoplugged decoder of the other class out of autoplug. Those of the class keep GStreamer's ranks, so its
    // order among them holds and one it ranks NONE (experimental, or unfit for the platform) stays out.
    void demoteOtherClass(bool hardware, QStringList& warnings)
    {
        const bool classAutoplugged = std::ranges::any_of(_decoders, [&](const DecoderInfo& decoder) {
            return (decoder.hardware == hardware) && autoplugged(decoder);
        });
        if (!_decoders.isEmpty() && !classAutoplugged) {
            warnings.append(QStringLiteral("No %1 video decoder that GStreamer autoplugs is installed: "
                                           "video will not decode with this option.")
                                .arg(hardware ? QStringLiteral("hardware") : QStringLiteral("software")));
        }
        (void) rank([&](const DecoderInfo& decoder) { return (decoder.hardware != hardware) && autoplugged(decoder); },
                    GST_RANK_NONE);
    }

    // The decoders whose DMABuf output the GPU paths import, above every other. A va decoder GStreamer ranks NONE
    // (vajpegdec, vah266dec) stays out of autoplug, except where it ranks them all NONE, as it did before 1.24.
    void prioritizeZeroCopy()
    {
        const auto va = [](const DecoderInfo& decoder) { return decoder.family == VideoDecoderOption::VAAPI; };
        const bool someAutoplugged = std::ranges::any_of(
            _decoders, [&](const DecoderInfo& decoder) { return va(decoder) && autoplugged(decoder); });
        (void) rank(
            [&](const DecoderInfo& decoder) { return va(decoder) && (!someAutoplugged || autoplugged(decoder)); },
            kZeroCopyRank);
        rankPrefix("v4l2sl", kZeroCopyRank);
    }

    // Every hardware decoder that would rank above @p rank, down to it.
    void lowerHardware(uint16_t rank)
    {
        for (qsizetype i = 0; i < _decoders.size(); ++i) {
            const DecoderInfo& decoder = _decoders.at(i);
            const unsigned planned = _ranks.at(i) ? *_ranks.at(i) : decoder.rank;
            if (decoder.hardware && (planned > rank)) {
                _ranks[i] = rank;
            }
        }
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

RankPlan planRanks(VideoDecoderOption option, const QList<DecoderInfo>& decoders, const RankTraits& traits)
{
    RankPlan plan;
    Planner planner(decoders);
    uint16_t familyRank = 0;

    switch (option) {
        case VideoDecoderOption::Default:
            if (traits.preferZeroCopyDecoders) {
                planner.rankPrefix("vaapi", GST_RANK_NONE);
                planner.prioritizeZeroCopy();
            }
            if (traits.preferVideoToolbox) {
                // Only out of the tie: one GStreamer ranks lower, or leaves out of autoplug, stays where it is.
                (void) planner.rank(
                    [](const DecoderInfo& decoder) {
                        return (decoder.family == VideoDecoderOption::VideoToolbox) &&
                               (decoder.rank == GST_RANK_PRIMARY);
                    },
                    kPrioritizedRank);
            }
            break;
        case VideoDecoderOption::Software:
            planner.demoteOtherClass(false, plan.warnings);
            break;
        case VideoDecoderOption::Hardware:
            planner.demoteOtherClass(true, plan.warnings);
            if (traits.preferZeroCopyDecoders) {
                // Decoders of one rank autoplug in name order, the legacy vaapi ones before the va ones. The vaapi
                // ones stay as the hardware fallback for a codec nothing else decodes.
                planner.prioritizeZeroCopy();
            }
            break;
        case VideoDecoderOption::VAAPI:
        case VideoDecoderOption::NVIDIA:
        case VideoDecoderOption::DirectX3D:
        case VideoDecoderOption::VideoToolbox:
        case VideoDecoderOption::Intel:
            familyRank = planner.prioritizeFamily(option);
            break;
    }

    if (traits.rhiD3DApi) {
        // A Direct3D 12 RHI samples d3d12 frames, a Direct3D 11 one d3d11 frames, and d3d12 ones too when its device
        // opens them.
        const bool sampledD3D12 = (*traits.rhiD3DApi == D3DApi::D3D12) || traits.d3d11OpensD3D12;
        const bool sampledD3D11 = (*traits.rhiD3DApi == D3DApi::D3D11);
        const char* const preferred = sampledD3D12 ? "d3d12" : "d3d11";
        const auto onRhiAdapter = [&traits](const DecoderInfo& decoder) {
            return (traits.rhiAdapterLuid == 0) || (decoder.adapterLuid == 0) ||
                   (decoder.adapterLuid == traits.rhiAdapterLuid);
        };
        const auto prefer = [&](uint16_t rank) {
            (void) planner.rank(
                [&](const DecoderInfo& decoder) { return decoder.name.startsWith(preferred) && onRhiAdapter(decoder); },
                rank);
        };
        const bool forced = (option == VideoDecoderOption::DirectX3D);
        if ((option == VideoDecoderOption::Default) || (option == VideoDecoderOption::Hardware)) {
            prefer(kZeroCopyRank);
        } else if (forced && sampledD3D12 && sampledD3D11) {
            // Forcing the family ranks all of it above every other decoder: the preferred API goes first within it.
            prefer(static_cast<uint16_t>(familyRank + 1));
        }
        if (forced || (option == VideoDecoderOption::Default) || (option == VideoDecoderOption::Hardware)) {
            (void) planner.rank(
                [&](const DecoderInfo& decoder) {
                    return (decoder.family == VideoDecoderOption::DirectX3D) && !onRhiAdapter(decoder);
                },
                GST_RANK_SECONDARY);
        }
        if (!sampledD3D12) {
            planner.rankPrefix("d3d12", GST_RANK_NONE);
        }
        if (!sampledD3D11) {
            planner.rankPrefix("d3d11", GST_RANK_NONE);
        }
    }

    if (traits.softwareRendering && (option == VideoDecoderOption::Default)) {
        // After the platform rules, which raise hardware decoders.
        planner.lowerHardware(GST_RANK_SECONDARY);
    }

    plan.changes = planner.changes();
    return plan;
}

}  // namespace GStreamer
