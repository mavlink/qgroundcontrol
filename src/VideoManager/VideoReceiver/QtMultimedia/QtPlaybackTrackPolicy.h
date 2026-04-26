#pragma once

class QMediaPlayer;

/// Video-only QGC streams do not need decoded audio/subtitle tracks. Keeping
/// the policy separate makes the receiver wiring testable without a player.
class QtPlaybackTrackPolicy final
{
public:
    struct Decision
    {
        bool disableAudio = false;
        bool disableSubtitles = false;
    };

    QtPlaybackTrackPolicy() = delete;

    [[nodiscard]] static Decision decisionFor(bool playbackMode, int audioTrackCount, int subtitleTrackCount);
    static Decision apply(QMediaPlayer* player, bool playbackMode);
};
