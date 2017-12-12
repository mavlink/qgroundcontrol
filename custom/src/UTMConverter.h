/*!
 *   @brief Typhoon H QGCCorePlugin Declaration
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#pragma once

#include <QObject>
#include "MAVLinkProtocol.h"

//-----------------------------------------------------------------------------
class UTMConverter : public QObject
{
    Q_OBJECT
public:
    UTMConverter(bool skyward);
    ~UTMConverter();

    bool        convertTelemetryFile        (const QString& srcFilename, const QString& dstFilename);

public slots:
    void        cancel                      ();

private:
    qint64      _curTimeUSecs;
    qint64      _startDTG;
    double      _lastSpeed;
    double      _lastBattery;
    bool        _gpsRawIntMessageAvailable;
    bool        _globalPositionIntMessageAvailable;
    int         _mavlinkChannel;
    QFile       _logFile;
    bool        _cancel;
    bool        _convertToSkyward;

    typedef struct {
        qint64  timeStamp;
        double  time;
        double  lon;
        double  lat;
        double  alt;
        double  speed;
        double  battery;
    } UTM_LogItem;

    QVector<UTM_LogItem> _logItems;

    QString     _cameraModel;
    QString     _cameraVersion;
    QString     _gimbalVersion;

    QString     _apUID;
    QString     _apVersion;

private:
    bool        _compareItem                (UTM_LogItem logItem1, UTM_LogItem logItem2);
    void        _newMavlinkMessage          (qint64 curTimeUSecs, mavlink_message_t message);
    void        _handleGlobalPositionInt    (mavlink_message_t& message);
    void        _handleGpsRawInt            (mavlink_message_t& message);
    void        _handleVfrHud               (mavlink_message_t& message);
    void        _handleBatteryStatus        (mavlink_message_t& message);
    void        _handleCameraInfo           (mavlink_message_t& message);
    void        _handleAutopilotVersion     (mavlink_message_t& message);
    quint64     _parseTimestamp             (const QByteArray& bytes);
    quint64     _readNextMavlinkMessage     (mavlink_message_t &message);
};
