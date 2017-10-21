/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include <QFile>
#include <QDateTime>

#include "TlogParser.h"
#include "MAVLinkProtocol.h"

//-----------------------------------------------------------------------------
class UTMConverter : public QObject
{
    Q_OBJECT
public:
    UTMConverter(QObject* parent = NULL);
    ~UTMConverter();

    bool        parseLogFile                (const QString& logFilename, const QString &path);

signals:
    void        completed                   ();

private slots:
    void        _newMavlinkMessage          (qint64 curTimeUSecs, mavlink_message_t message);
    void        _completed                  ();

private:
    QString     _logFileName;
    QFile       _utmLogFile;
    TlogParser* _parser;
    qint64      _curTimeUSecs;
    qint64      _startDTG;
    double      _lastSpeed;
    bool        _gpsRawIntMessageAvailable;
    bool        _globalPositionIntMessageAvailable;

    typedef struct {
        double  time;
        double  lon;
        double  lat;
        double  alt;
        double  speed;
    } UTM_LogItem;

    QVector<UTM_LogItem> _logItems;

private:
    void        _reset                      ();
    bool        _compareItem                (UTM_LogItem logItem1, UTM_LogItem logItem2);
    void        _handleGlobalPositionInt    (mavlink_message_t& message);
    void        _handleGpsRawInt            (mavlink_message_t& message);
    void        _handleVfrHud               (mavlink_message_t& message);

};
