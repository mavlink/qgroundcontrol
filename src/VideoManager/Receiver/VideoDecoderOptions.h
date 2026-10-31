#pragma once

/// The decoders VideoSettings' forceVideoDecoder can force. The values are persisted.
namespace VideoDecoder {

enum Options : int
{
    ForceVideoDecoderDefault = 0,
    ForceVideoDecoderSoftware,
    ForceVideoDecoderNVIDIA,
    ForceVideoDecoderVAAPI,
    ForceVideoDecoderDirectX3D,
    ForceVideoDecoderVideoToolbox,
    ForceVideoDecoderIntel,
    ForceVideoDecoderVulkan,
    ForceVideoDecoderHardware
};

}  // namespace VideoDecoder
