#include "QtPlaybackTrackPolicy.h"

#include <QtMultimedia/QMediaMetaData>
#include <QtMultimedia/QMediaPlayer>

QtPlaybackTrackPolicy::Decision QtPlaybackTrackPolicy::decisionFor(bool playbackMode,
                                                                   int audioTrackCount,
                                                                   int subtitleTrackCount)
{
    if (!playbackMode)
        return {};

    return {
        .disableAudio = audioTrackCount > 0,
        .disableSubtitles = subtitleTrackCount > 0,
    };
}

QtPlaybackTrackPolicy::Decision QtPlaybackTrackPolicy::apply(QMediaPlayer* player, bool playbackMode)
{
    if (!player)
        return {};

    const Decision decision = decisionFor(playbackMode, player->audioTracks().size(), player->subtitleTracks().size());
    if (decision.disableAudio)
        player->setActiveAudioTrack(-1);
    if (decision.disableSubtitles)
        player->setActiveSubtitleTrack(-1);
    return decision;
}
