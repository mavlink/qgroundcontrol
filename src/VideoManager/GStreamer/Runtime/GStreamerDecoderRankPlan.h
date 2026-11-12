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
    VideoDecoderOption family = VideoDecoderOption::Default;
    /// The DXGI adapter a Direct3D decoder decodes on; 0 for every other decoder, and when it isn't known.
    qint64 adapterLuid = 0;
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
    /// Linux: for the default and hardware options, rank the va and v4l2sl decoders, which output DMABuf, above every
    /// other. The default option also demotes the legacy vaapi decoders, whose memory the GPU paths can't import; the
    /// hardware option keeps them below as a fallback.
    bool preferZeroCopyDecoders = false;
    /// Windows: the Direct3D API Qt renders with. After any option, the D3D decoders whose frames it can't sample are
    /// demoted; for the default and hardware options, those it prefers rank above every other.
    std::optional<D3DApi> rhiD3DApi;
    /// Windows, Direct3D 11: Qt's device opens a Direct3D 12 device's resources (HwBuffers::d3d11OpensD3D12()). The
    /// d3d12 decoders are then the ones preferred, as GStreamer ranks them: they decode on a device of their own, not
    /// on the one Qt draws with. The d3d11 ones keep GStreamer's rank, behind them.
    bool d3d11OpensD3D12 = false;
    /// Windows: the adapter Qt renders on; 0 while it isn't known. A D3D decoder on another adapter ranks below the
    /// software decoders: its frames reach Qt through the CPU too, and from another GPU.
    qint64 rhiAdapterLuid = 0;
    /// The user asked for software rendering (--swrast, the GPU Safe Mode shortcut), to keep off a GPU or driver that
    /// misbehaves. Qt samples no decoder's GPU frame then: with the default option every hardware decoder ranks
    /// below the software decoders. A decoder option the user chose stays as it is.
    bool softwareRendering = false;

    friend bool operator==(const RankTraits&, const RankTraits&) = default;
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
RankPlan planRanks(VideoDecoderOption option, const QList<DecoderInfo>& decoders, const RankTraits& traits);

}  // namespace GStreamer
