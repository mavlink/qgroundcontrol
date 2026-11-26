#pragma once

#include <cstdint>

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>

#include "StreamSupervisor.h"
#include "StreamWatchdog.h"
#include "VideoReceiver.h"
#include "VideoStreamStatsSampler.h"
#include "VideoStreamUri.h"

/// A VideoReceiver that plays its stream through FFmpeg, in sessions it runs from its own thread: what the Qt
/// Multimedia and FFmpeg receivers share. A session is one connection to the stream, opened for what
/// VideoStreamUri::ffmpegSource() makes of the config's URI. The backend opens and closes it, shows its pictures and
/// records them; this class decides when:
///
/// - A session runs while the receiver is active with a URI FFmpeg can open. A user start, stop or restart, a config
///   with another stream, or the backend (startOver()) starts it over.
/// - A session that fails (fail()), or that its StreamWatchdog finds stalled, ends. The StreamSupervisor then
///   reconnects after 1 s, doubling up to 30 s, while autoReconnect is on, and the stream stops with errorOccurred()
///   otherwise, which also ends its recording. A UDP port nobody sends to yet hasn't failed: once FFmpeg gives up on
///   it, a new session listens at once.
/// - Once a second while a session runs, the watchdog is asked, and then the backend for its stats (sampleSession()).
///
/// The watchdog's sink frames are the pictures at the output, which the backend notes (_watchdog.noteSinkFrame()): one
/// is due within the config's timeout while pictureDue(), 5 s more for a session's first. Its source frames are the
/// stream's packets, for a backend that notes them: without one for the timeout the stream has stalled, and a server
/// has 5 s more for its first.
class SessionVideoReceiver : public VideoReceiver
{
    Q_OBJECT

protected:
    /// The backend's logging category: Q_DECLARE_LOGGING_CATEGORY()'s function.
    using LogCategory = const QLoggingCategory& (*)();

    /// Whether the backend notes the stream's packets as they arrive (_watchdog.noteSourceFrame()).
    enum class PacketsNoted : uint8_t
    {
        No,
        Yes,
    };

    SessionVideoReceiver(LogCategory log, PacketsNoted packetsNoted, QObject* parent);

    void onConfigChanged() final;
    void onActiveChanged() final;
    void onRestartRequested() final;
    void onReconnectRequested() final;
    void onStartRecording(const QString& file, FileFormat format, const RecordingEncoding& encoding) final;
    void onStopRecording() final;

    /// A session is open.
    virtual bool hasSession() const = 0;
    /// Has open() called for the session start() started: now, or once what the last session held is free.
    virtual void openWhenReady() = 0;
    /// Opens a session that plays @p source as _sessionConfig says. The watchdog watches it already.
    virtual void openSession(const VideoStreamUri::FFmpegSource& source) = 0;
    /// Ends the session, if any, without waiting for it: no picture from it any more, and what it counted is forgotten.
    virtual void closeSession() = 0;
    /// The session owes the output pictures.
    virtual bool pictureDue() const = 0;
    /// Once a second while a session runs that hasn't stalled: its stats, and what else it does that often.
    virtual void sampleSession() = 0;
    /// startRecording() of a stream that hasn't stopped for good.
    virtual void beginRecording(const QString& file, FileFormat format, const RecordingEncoding& encoding) = 0;
    /// Ends the recording, if any.
    virtual void endRecording() = 0;

    /// A user start, stop or restart, or a new config: the backoff and the reconnect count start over.
    virtual void startOver();
    /// Starts a new session when the stream should run; stops it otherwise.
    void start();
    /// Opens the session start() started, unless the stream stopped meanwhile: for openWhenReady().
    void open();
    /// The session failed for @p reason: reconnects with backoff, or stops and reports it; a @p permanent failure
    /// always stops. One still awaiting its sender starts anew instead.
    void fail(const QString& reason, bool permanent = false);
    /// The URI without its credentials, for logs and messages.
    QString redactedUri() const;

    StreamSupervisor* const _supervisor;
    QTimer _sampleTimer;
    VideoStreamStatsSampler _statsSampler;
    /// The config the session started with.
    Config _sessionConfig;
    /// Of the current session.
    StreamWatchdog _watchdog;

private:
    /// Ends the session: no picture or stats.
    void _stopSession();
    /// The session listens on a UDP port, for at least its timeout, and no stream has arrived.
    bool _awaitingSender() const;
    /// The stream has stopped for good, until it is started over: ends its recording and reports @p message.
    void _onHalted(const QString& message);
    void _sample();

    const LogCategory _log;
    const PacketsNoted _packetsNoted;
    /// Since the session was opened.
    QElapsedTimer _sinceOpened;
};
