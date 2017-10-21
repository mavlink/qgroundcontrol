/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include "MAVLinkProtocol.h"

//-----------------------------------------------------------------------------
class UTMConverter
{
public:
    UTMConverter();
    ~UTMConverter();

    bool        convertTelemetryFile        (const QString& srcFilename, const QString& dstFilename);

private:
    qint64      _curTimeUSecs;
    qint64      _startDTG;
    double      _lastSpeed;
    bool        _gpsRawIntMessageAvailable;
    bool        _globalPositionIntMessageAvailable;
    int         _mavlinkChannel;
    QFile       _logFile;

    typedef struct {
        double  time;
        double  lon;
        double  lat;
        double  alt;
        double  speed;
    } UTM_LogItem;

    QVector<UTM_LogItem> _logItems;

private:
    bool        _compareItem                (UTM_LogItem logItem1, UTM_LogItem logItem2);
    void        _newMavlinkMessage          (qint64 curTimeUSecs, mavlink_message_t message);
    void        _handleGlobalPositionInt    (mavlink_message_t& message);
    void        _handleGpsRawInt            (mavlink_message_t& message);
    void        _handleVfrHud               (mavlink_message_t& message);
    quint64     _parseTimestamp             (const QByteArray& bytes);
    quint64     _readNextMavlinkMessage     (mavlink_message_t &message);
};
