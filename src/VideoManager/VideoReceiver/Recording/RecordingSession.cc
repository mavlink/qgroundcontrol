#include "RecordingSession.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QSaveFile>
#include <QtCore/QStorageInfo>
#include <QtCore/QTimer>

#include "QGCLoggingCategory.h"
#include "RecordingOrphanScanner.h"
#include "VideoRecorder.h"

QGC_LOGGING_CATEGORY(RecordingSessionLog, "Video.RecordingSession")

// ─── Helpers ─────────────────────────────────────────────────────────────────

static QString formatName(QMediaFormat::FileFormat fmt)
{
    switch (fmt) {
        case QMediaFormat::Matroska:  return QStringLiteral("mkv");
        case QMediaFormat::QuickTime: return QStringLiteral("mov");
        case QMediaFormat::MPEG4:     return QStringLiteral("mp4");
        default:                      return QStringLiteral("unknown");
    }
}

// ─── Constructor / Destructor ─────────────────────────────────────────────────

RecordingSession::RecordingSession(QObject* parent) : QObject(parent) {}

RecordingSession::~RecordingSession()
{
    if (isActive())
        stop();
}

// ─── start() ─────────────────────────────────────────────────────────────────

bool RecordingSession::start(const QString& recordingDir,
                             std::vector<StreamEntry>&& streams,
                             const QString& subtitleFile,
                             const QString& vehicleUid)
{
    if (isActive()) {
        qCWarning(RecordingSessionLog) << "Session already active";
        // Streams passed in are dropped here (unique_ptr destroys recorders).
        return false;
    }

    if (streams.empty()) {
        qCWarning(RecordingSessionLog) << "No streams provided";
        return false;
    }

    _recordingDir   = recordingDir;
    _streams        = std::move(streams);
    _subtitleFile   = subtitleFile;
    _vehicleUid     = vehicleUid;
    const QDateTime startUtc = QDateTime::currentDateTimeUtc();
    _startTimestampIso = startUtc.toString(Qt::ISODateWithMs);
    _stopTimestampIso.clear();
    _activeRecorders = 0;

    // Ensure recording dir exists
    QDir().mkpath(recordingDir);

    // Disk-space preflight: fail fast rather than letting the muxer run out
    // of space mid-recording, which leaves a truncated file the orphan scanner
    // has to triage. 256 MiB is a conservative floor — smaller than any
    // meaningful recording at typical drone bitrates.
    constexpr qint64 kMinAvailableBytes = 256LL * 1024 * 1024;
    const QStorageInfo storage(recordingDir);
    if (storage.isValid() && storage.isReady()
        && storage.bytesAvailable() >= 0
        && storage.bytesAvailable() < kMinAvailableBytes) {
        qCWarning(RecordingSessionLog)
            << "Insufficient disk space in" << recordingDir
            << "available:" << storage.bytesAvailable() << "bytes";
        emit error(tr("Insufficient disk space for recording (%1 MiB available)")
                       .arg(storage.bytesAvailable() / (1024 * 1024)));
        return false;
    }

    // Filesystem-safe manifest name: ISO-like but with '-' instead of ':' and '.'.
    const QString safeTs = startUtc.toString(QStringLiteral("yyyy-MM-ddTHH-mm-ss-zzz"));
    _manifestPath = recordingDir + QStringLiteral("/session-") + safeTs + QStringLiteral(".json");

    if (!_writeManifest(false)) {
        _manifestPath.clear();
        emit error(QStringLiteral("Failed to write recording manifest"));
        return false;
    }

    emit manifestPathChanged();
    emit activeChanged();

    // Start each recorder; roll back on any failure. Raw pointers in the
    // rollback list are non-owning — ownership stays in _streams.
    QList<VideoRecorder*> startedRecorders;
    for (const auto& entry : std::as_const(_streams)) {
        VideoRecorder* r = entry.recorder.get();
        if (!r) {
            qCWarning(RecordingSessionLog) << "Null recorder for role" << entry.role;
            continue;
        }

        auto conn1 = connect(r, &VideoRecorder::stopped,
                             this, &RecordingSession::_onRecorderStopped);
        auto conn2 = connect(r, &VideoRecorder::error,
                             this, &RecordingSession::_onRecorderError);
        _recorderConns << conn1 << conn2;

        if (!r->start(entry.path, entry.format)) {
            qCWarning(RecordingSessionLog) << "Failed to start recorder for" << entry.role;
            // Roll back everything started so far
            for (auto* started : startedRecorders)
                started->stop();
            for (const auto& c : std::as_const(_recorderConns))
                disconnect(c);
            _recorderConns.clear();
            QFile::remove(_manifestPath);
            _manifestPath.clear();
            // Clearing _streams destroys the unique_ptrs → recorders deleted.
            _streams.clear();
            emit activeChanged();
            emit manifestPathChanged();
            return false;
        }

        startedRecorders.append(r);
        ++_activeRecorders;
    }

    qCInfo(RecordingSessionLog) << "Session started:" << _manifestPath
                                << "streams:" << _activeRecorders;
    emit started();
    return true;
}

// ─── stop() ──────────────────────────────────────────────────────────────────

void RecordingSession::stop()
{
    if (!isActive())
        return;

    qCInfo(RecordingSessionLog) << "Stopping session" << _manifestPath;

    for (const auto& entry : std::as_const(_streams)) {
        if (entry.recorder && entry.recorder->isRecording())
            entry.recorder->stop();
    }
    // NOTE: _streams is NOT cleared here — we still need the recorders to be
    // alive until _onRecorderStopped decrements _activeRecorders to zero and
    // _finalizeAndCleanup() clears the list (which destroys the unique_ptrs).

    // If recorder stops were synchronous (StubRecorder in tests, or BridgeRecorder
    // during teardown with no pending frames), _onRecorderStopped already ran
    // _finalizeAndCleanup which cleared _manifestPath. isActive() checks that path,
    // so this guards against double-finalize.
    if (_activeRecorders <= 0 && isActive())
        _finalizeAndCleanup();
}

// ─── Private: finalize ───────────────────────────────────────────────────────

void RecordingSession::_onRecorderStopped(const QString& /*path*/)
{
    --_activeRecorders;
    qCDebug(RecordingSessionLog) << "Recorder stopped; remaining:" << _activeRecorders;

    if (_activeRecorders <= 0)
        _finalizeAndCleanup();
}

void RecordingSession::_onRecorderError(const QString& message)
{
    qCWarning(RecordingSessionLog) << "Recorder error:" << message;
    emit error(message);
    // Don't abort — let the remaining recorders finish
}

void RecordingSession::_finalizeAndCleanup()
{
    for (const auto& c : std::as_const(_recorderConns))
        disconnect(c);
    _recorderConns.clear();

    _stopTimestampIso = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);

    // Write final manifest with stop timestamp, then delete it (clean completion).
    _writeManifest(true);
    QFile::remove(_manifestPath);

    const QString oldPath = _manifestPath;
    _manifestPath.clear();
    _streams.clear();
    _activeRecorders = 0;

    emit manifestPathChanged();
    emit activeChanged();
    emit stopped();

    qCInfo(RecordingSessionLog) << "Session completed cleanly; manifest removed.";
}

// ─── _writeManifest ──────────────────────────────────────────────────────────

bool RecordingSession::_writeManifest(bool includeStopTimestamp)
{
    QJsonObject root;
    root[QStringLiteral("version")]        = 1;
    root[QStringLiteral("startTimestamp")] = _startTimestampIso;
    root[QStringLiteral("stopTimestamp")]  = includeStopTimestamp
                                              ? QJsonValue(_stopTimestampIso)
                                              : QJsonValue(QJsonValue::Null);
    root[QStringLiteral("vehicleUid")]     = _vehicleUid;
    root[QStringLiteral("subtitleFile")]   = _subtitleFile;

    QJsonArray streamsArr;
    for (const auto& entry : std::as_const(_streams)) {
        QJsonObject s;
        s[QStringLiteral("role")]   = entry.role;
        s[QStringLiteral("path")]   = entry.path;
        s[QStringLiteral("format")] = formatName(entry.format);
        s[QStringLiteral("status")] = includeStopTimestamp
                                      ? QStringLiteral("ok")
                                      : QStringLiteral("recording");
        streamsArr.append(s);
    }
    root[QStringLiteral("streams")] = streamsArr;

    const QByteArray json = QJsonDocument(root).toJson();

    QSaveFile f(_manifestPath);
    if (!f.open(QIODevice::WriteOnly)) {
        qCCritical(RecordingSessionLog) << "Cannot open manifest for writing:" << _manifestPath;
        return false;
    }
    f.write(json);
    if (!f.commit()) {
        qCCritical(RecordingSessionLog) << "Failed to commit manifest:" << _manifestPath;
        return false;
    }
    return true;
}

int RecordingSession::scanForOrphans(const QString& recordingDir, QObject* notifyParent)
{
    // Forward to RecordingOrphanScanner; re-emit corruptionDetected when the
    // parent is a RecordingSession so existing QML/Qt consumers keep working.
    RecordingOrphanScanner::CorruptionCallback cb;
    if (auto* session = qobject_cast<RecordingSession*>(notifyParent)) {
        cb = [session](const QStringList& movedFiles) {
            QMetaObject::invokeMethod(session, [session, movedFiles]() {
                emit session->corruptionDetected(movedFiles);
            });
        };
    }
    return RecordingOrphanScanner::scan(recordingDir, cb);
}
