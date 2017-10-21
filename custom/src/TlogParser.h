/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QFile>

#include "MAVLinkProtocol.h"

//-----------------------------------------------------------------------------
class TlogParser : public QThread
{
    Q_OBJECT
public:
    TlogParser();
    ~TlogParser();

    bool        parseLogFile                (const QString& logFilename);

protected:
    void        run                         ();

signals:
    void        newMavlinkMessage           (qint64 curTimeUSecs, mavlink_message_t message);
    void        completed                   ();

private slots:

private:
    quint64     _parseTimestamp             (const QByteArray& bytes);
    quint64     _readNextMavlinkMessage     (mavlink_message_t &message);

private:
    QFile       _logFile;
    qint64      _curTimeUSecs;
    int         _mavlinkChannel;
};
