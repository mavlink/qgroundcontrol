/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "UTMConverter.h"
#include <QFileInfo>

const char* kLoggingHeader =
"{\n"\
"    \"exchange\": {\n"\
"        \"exchange_type\": \"flight_logging\",\n"\
"        \"message\": {\n"\
"            \"flight_logging\": {\n"\
"                \"flight_logging_items\": [\n";

const char* kLoggingKeys =
"                ],\n"\
"                \"flight_logging_keys\": [\n"\
"                    \"timestamp\", \"gps_lon\", \"gps_lat\", \"gps_altitude\", \"speed\"\n"\
"                ],\n"\
"                \"altitude_system\": \"WGS84\",\n";

const char* kLoggingFooter =
"        \"file\": {\n"\
"            \"logging_type\": \"GUTMA_DX_JSON\",\n"\
"            \"filename\": \"###FILENAME###\",\n"\
"            \"creation_dtg\": \"###FILEDATE###Z\"\n"\
"            },\n"\
"           \"message_type\": \"flight_logging_submission\"\n"\
"        }\n"\
"    }\n"\
"}\n";

//-----------------------------------------------------------------------------
UTMConverter::UTMConverter(QObject* parent)
    : QObject(parent)
    , _parser(NULL)
{
    _reset();
}

//-----------------------------------------------------------------------------
UTMConverter::~UTMConverter()
{
    if(_parser) {
        _parser->deleteLater();
    }
}

//-----------------------------------------------------------------------------
bool
UTMConverter::parseLogFile(const QString& logFilename, const QString& path)
{
    if (_utmLogFile.isOpen()) {
        qWarning() << "Attempting to parse an already open log file.";
        return false;
    }
    QFileInfo fi(logFilename);
    _logFileName = QString("%1/%2.utm").arg(path).arg(fi.baseName());
    _utmLogFile.setFileName(_logFileName);
    if (!_utmLogFile.open(QFile::WriteOnly)) {
        qWarning() << QString("Unable to create log file: '%1', error: %2").arg(_logFileName).arg(_utmLogFile.errorString());
        return false;
    }
    _reset();
    _parser = new TlogParser();
    connect(_parser, &TlogParser::completed, this, &UTMConverter::_completed);
    connect(_parser, &TlogParser::newMavlinkMessage, this, &UTMConverter::_newMavlinkMessage);
    if(!_parser->parseLogFile(logFilename)) {
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
void
UTMConverter::_reset()
{
    _curTimeUSecs   = 0;
    _startDTG       = 0;
    _lastSpeed      = 0;
    _gpsRawIntMessageAvailable = false;
    _globalPositionIntMessageAvailable = false;
}

//-----------------------------------------------------------------------------
void
UTMConverter::_completed()
{
    if(_parser) {
        _parser->deleteLater();
        _parser = NULL;
    }
    if(_logItems.size()) {
        //-- Header
        _utmLogFile.write(kLoggingHeader);
        for(int i = 0; i < _logItems.size(); i++) {
            QString line;
            line.sprintf("                %.3f, %f, %f, %.3f, %.3f ]",
                _logItems[i].time,
                _logItems[i].lon,
                _logItems[i].lat,
                _logItems[i].alt,
                _logItems[i].speed);
            if(i < _logItems.size() - 1) {
                line += ",\n";
            } else {
                line += "\n";
            }
            _utmLogFile.write(line.toLocal8Bit());
        }
        _utmLogFile.write(kLoggingKeys);
        QDateTime dtg = QDateTime::fromMSecsSinceEpoch(_startDTG / 1000);
        QString line = QString("                \"logging_start_dtg\": \"%1Z\"\n").arg(dtg.toString(Qt::ISODate));
        _utmLogFile.write(line.toLocal8Bit());
        QString footer(kLoggingFooter);
        QFileInfo fi(_logFileName);
        footer.replace("###FILENAME###", fi.baseName());
        footer.replace("###FILEDATE###", QDateTime::currentDateTime().toString(Qt::ISODate));
        _utmLogFile.write(footer.toLocal8Bit());
    }
    _utmLogFile.close();
    if(!_logItems.size()) {
        _utmLogFile.remove();
    }
    emit completed();
}

//-----------------------------------------------------------------------------
void
UTMConverter::_newMavlinkMessage(qint64 curTimeUSecs, mavlink_message_t message)
{
    //-- First Message
    if(!_startDTG) {
        _startDTG = curTimeUSecs;
    }
    _curTimeUSecs = curTimeUSecs;
    switch(message.msgid) {
    case MAVLINK_MSG_ID_GPS_RAW_INT:
        _handleGpsRawInt(message);
        break;
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
        _handleGlobalPositionInt(message);
        break;
    case MAVLINK_MSG_ID_VFR_HUD:
        _handleVfrHud(message);
        break;
    }
    //-- TODO: Need to handle mode changes to capture events
}

//-----------------------------------------------------------------------------
bool
UTMConverter::_compareItem(UTM_LogItem logItem1, UTM_LogItem logItem2)
{
    if(logItem1.lon   != logItem2.lon)
        return false;
    if(logItem1.lat   != logItem2.lat)
        return false;
    if(logItem1.alt   != logItem2.alt)
        return false;
    if(logItem1.speed != logItem2.speed)
        return false;
    return true;
}

//-----------------------------------------------------------------------------
void
UTMConverter::_handleGpsRawInt(mavlink_message_t& message)
{
    _gpsRawIntMessageAvailable = true;
    if (!_globalPositionIntMessageAvailable) {
        mavlink_gps_raw_int_t gpsRawInt;
        mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);
        if (gpsRawInt.fix_type >= GPS_FIX_TYPE_3D_FIX) {
            UTM_LogItem logItem;
            logItem.lon   = gpsRawInt.lat / (double)1E7;
            logItem.lat   = gpsRawInt.lat / (double)1E7;
            logItem.alt   = gpsRawInt.alt / 1000.0;
            logItem.time  = (_curTimeUSecs / 1000000) - (_startDTG / 1000000);
            logItem.speed = _lastSpeed;
            if(_logItems.size()) {
                if(!_compareItem(_logItems[_logItems.size()-1], logItem)) {
                    _logItems.append(logItem);
                }
            } else {
                _logItems.append(logItem);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
UTMConverter::_handleVfrHud(mavlink_message_t& message)
{
    mavlink_vfr_hud_t vfrHud;
    mavlink_msg_vfr_hud_decode(&message, &vfrHud);
    _lastSpeed = qIsNaN(vfrHud.groundspeed) ? 0 : vfrHud.groundspeed;
}

//-----------------------------------------------------------------------------
void
UTMConverter::_handleGlobalPositionInt(mavlink_message_t& message)
{
    _globalPositionIntMessageAvailable = true;
    if(!_gpsRawIntMessageAvailable) {
        mavlink_global_position_int_t globalPositionInt;
        mavlink_msg_global_position_int_decode(&message, &globalPositionInt);
        UTM_LogItem logItem;
        logItem.lon   = globalPositionInt.lon / (double)1E7;
        logItem.lat   = globalPositionInt.lat / (double)1E7;
        logItem.alt   = globalPositionInt.alt / 1000.0;
        logItem.time  = (_curTimeUSecs / 1000000) - (_startDTG / 1000000);
        logItem.speed = _lastSpeed;
        if(_logItems.size()) {
            if(!_compareItem(_logItems[_logItems.size()-1], logItem)) {
                _logItems.append(logItem);
            }
        } else {
            _logItems.append(logItem);
        }
    }
}

