#pragma once

#include <cstdint>
#include <optional>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QStringList>

#include "VideoDecoderOptions.h"

// The decoder rank policy behind the forceVideoDecoder setting, as a pure function of the installed decoders: no
// registry, so every platform's rules can be tested on any host. GStreamerDecoderRanks applies its plans.
namespace GStreamer {

/// A video decoder factory as planRanks() sees it.
struct DecoderInfo
{
    QByteArray name;
    unsigned rank = 0;  ///< The rank before QGC changed it.
    bool hardware = false;
    VideoDecoder::Options family = VideoDecoder::ForceVideoDecoderDefault;
};

/// The D3D decoder API a Windows QRhi backend samples zero-copy.
enum class D3DApi : uint8_t
{
    D3D11,
    D3D12,
};

/// The platform rules planRanks() adds to an option.
struct RankTraits
{
    /// Linux: for the default option, demote the legacy vaapi decoders, whose memory the GPU paths can't import, and
    /// rank the va and v4l2sl decoders, which output DMABuf, above the software ones.
    bool preferZeroCopyDecoders = false;
    /// Windows: after any option, demote the D3D decoders of the other API, which the RHI can't sample zero-copy; for
    /// the default and hardware options, also rank this API's decoders above every other.
    std::optional<D3DApi> rhiD3DApi;
};

struct RankChange
{
    QByteArray feature;
    uint16_t rank = 0;

    friend bool operator==(const RankChange&, const RankChange&) = default;
};

struct RankPlan
{
    /// At most one per decoder, in the order of planRanks()'s input.
    QList<RankChange> changes;
    /// Why the option may not do what it says, for the log.
    QStringList warnings;
};

/// The rank changes @p option makes to @p decoders, from their original ranks, on a platform with @p traits.
RankPlan planRanks(VideoDecoder::Options option, const QList<DecoderInfo>& decoders, const RankTraits& traits);

}  // namespace GStreamer
