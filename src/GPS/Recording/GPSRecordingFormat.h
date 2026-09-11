#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <functional>
#include <optional>

class QIODevice;

#include "GPSRecordingEvent.h"
#include "GPSTransportResult.h"
#include "GPSType.h"

struct GPSReceiverProfile;

/// Versioned wire contract shared by capture and replay. Decode validates every stream before selection.
struct GPSRecordingDocument
{
    static constexpr int CURRENT_VERSION = 4;
    static constexpr qsizetype MAX_BYTES = 4 * 1024 * 1024;
    static constexpr qsizetype MAX_EVENTS = 100000;

    QVector<GPSRecordingEvent> events;
    bool limitReached = false;
    int sourceVersion = CURRENT_VERSION;

    /// The callback receives completed event count; false cancels between events.
    /// Failure can leave a partial document: callers publishing files must use QSaveFile.
    bool writeTo(QIODevice& device, QString& error, const std::function<bool(qsizetype)>& progress = {}) const;
    QByteArray encode(QString* error = nullptr) const;
    static bool decode(const QByteArray& bytes, GPSRecordingDocument& result, QString& error);
    bool selectStream(quint64 requestedStream, QVector<GPSRecordingEvent>& selected,
                      std::optional<GPSRecordingMetadata>& metadata, quint64& selectedStream, QString& error) const;
};
