#pragma once

/// The decoders VideoSettings' forceVideoDecoder can force. The values are persisted.
enum class VideoDecoderOption : int
{
    Default = 0,
    Software = 1,
    NVIDIA = 2,
    VAAPI = 3,
    DirectX3D = 4,
    VideoToolbox = 5,
    Intel = 6,
    Hardware = 8,
};

/// The option a stored forceVideoDecoder value names: the default for a value that names none, as 7 does, which
/// forced the Vulkan decoders.
constexpr VideoDecoderOption videoDecoderOptionFromSetting(int value)
{
    const bool valid = (value >= static_cast<int>(VideoDecoderOption::Default)) &&
                       (value <= static_cast<int>(VideoDecoderOption::Hardware)) && (value != 7);
    return valid ? static_cast<VideoDecoderOption>(value) : VideoDecoderOption::Default;
}
