#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QMetaType>
#include <QtCore/QString>

struct GPSEvent {
    Q_GADGET

public:
    enum class Severity { Info, Warning, Error };
    Q_ENUM(Severity)

    enum class Source { RTKBase, NTRIP, Transport, GPS, GCS };
    Q_ENUM(Source)

    // Uses system clock UTC; assumes host clock is reasonably synchronized
    QDateTime timestamp;
    Severity severity = Severity::Info;
    Source source = Source::GPS;
    QString message;

    static GPSEvent info(Source src, const QString& msg)    { return {QDateTime::currentDateTimeUtc(), Severity::Info, src, msg}; }
    static GPSEvent warning(Source src, const QString& msg) { return {QDateTime::currentDateTimeUtc(), Severity::Warning, src, msg}; }
    static GPSEvent error(Source src, const QString& msg)   { return {QDateTime::currentDateTimeUtc(), Severity::Error, src, msg}; }
};
Q_DECLARE_METATYPE(GPSEvent)
