/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "UTMConverter.h"
#include "QGCApplication.h"
#include "LinkManager.h"

#define TIMESTAMP_SIZE sizeof(quint64)

const char* kUTMLoggingHeader =
"{\n"\
"\t\"exchange\": {\n"\
"\t\t\"exchange_type\": \"flight_logging\",\n"\
"\t\t\"message\": {\n"\
"\t\t\t\"flight_data\": {\n"\
"\t\t\t\t\"aircraft\": {\n"\
"\t\t\t\t\t\"firmware_version\": \"###APFIRMWARE###\",\n"\
"\t\t\t\t\t\"manufacturer\": \"Yuneec\",\n"\
"\t\t\t\t\t\"model\": \"H520\",\n"\
"\t\t\t\t\t\"serial_number\": \"###APSERIAL###\"\n"\
"\t\t\t\t},\n"\
"\t\t\t\t\"gcs\": {\n"\
"\t\t\t\t\t\"manufacturer\": \"Yuneec\",\n"\
"\t\t\t\t\t\"model\": \"DataPilot\",\n"\
"\t\t\t\t\t\"version\": \"###DPVERSION###\"\n"\
"\t\t\t\t},\n"\
"\t\t\t\t\"payload\": [\n"\
"\t\t\t\t\t{\n"\
"\t\t\t\t\t\t\"firmware_version\": \"###CAMERAFIRMWARE###\",\n"\
"\t\t\t\t\t\t\"model\": \"###CAMERANAME###\"\n"\
"\t\t\t\t\t}\n"\
"\t\t\t\t]\n"\
"\t\t\t},\n"\
"\t\t\t\"flight_logging\": {\n"\
"\t\t\t\t\"flight_logging_items\": [\n";

const char* kSKYLoggingHeader =
"{\n"\
"\t\"exchange\": {\n"\
"\t\t\"exchange_type\": \"flight_logging\",\n"\
"\t\t\"exchanger\": \"DataPilot ###DPVERSION###\",\n"\
"\t\t\"flight_session_id\": \"0\",\n"\
"\t\t\"message\": {\n"\
"\t\t\t\"flight_data\": {\n"\
"\t\t\t\t\"flight_session_start\": \"###FLIGHTSTART###Z\",\n"\
"\t\t\t\t\"flight_session_end\": \"###FLIGHTEND###Z\",\n"\
"\t\t\t\t\"aircraft\": {\n"\
"\t\t\t\t\t\"firmware_version\": \"###APFIRMWARE###\",\n"\
"\t\t\t\t\t\"serial_number\": \"###APSERIAL###\",\n"\
"\t\t\t\t\t\"hardware_version\": \"1.0\",\n"\
"\t\t\t\t\t\"manufacturer\": \"Yuneec\",\n"\
"\t\t\t\t\t\"model\": \"H520\",\n"\
"\t\t\t\t\t\"name\": \"\"\n"\
"\t\t\t\t},\n"\
"\t\t\t\t\"gcs\": {\n"\
"\t\t\t\t\t\"manufacturer\": \"Yuneec\",\n"\
"\t\t\t\t\t\"model\": \"DataPilot\",\n"\
"\t\t\t\t\t\"version\": \"###DPVERSION###\"\n"\
"\t\t\t\t},\n"\
"\t\t\t\t\"logfile_device_origin\": {\n"\
"\t\t\t\t\t\"model\": \"ST16S\",\n"\
"\t\t\t\t\t\"operating_system\": \"Android 4.4.1\",\n"\
"\t\t\t\t\t\"user_interface_idiom\": \"Android\",\n"\
"\t\t\t\t\t\"device_ssid\": \"\"\n"\
"\t\t\t\t},\n"\
"\t\t\t\t\"remote_controller\": {\n"\
"\t\t\t\t\t\"firmware_version\": \"\",\n"\
"\t\t\t\t\t\"serial_number\": \"\"\n"\
"\t\t\t\t},\n"\
"\t\t\t\t\"flight_controller\": {\n"\
"\t\t\t\t\t\"firmware_version\": \"###APFIRMWARE###\",\n"\
"\t\t\t\t\t\"serial_number\": \"###APSERIAL###\"\n"\
"\t\t\t\t},\n"\
"\t\t\t\t\"payload\": {\n"\
"\t\t\t\t\t\"gimbal\": {\n"\
"\t\t\t\t\t\t\"firmware_version\": \"###GIMBALFIRMWARE###\",\n"\
"\t\t\t\t\t\t\"serial_number\": \"\",\n"\
"\t\t\t\t\t\t\"hardware_version\": \"\",\n"\
"\t\t\t\t\t\t\"model\": \"\"\n"\
"\t\t\t\t\t},\n"\
"\t\t\t\t\t\"camera\": {\n"\
"\t\t\t\t\t\t\"firmware_version\": \"###CAMERAFIRMWARE###\",\n"\
"\t\t\t\t\t\t\"serial_number\": \"\",\n"\
"\t\t\t\t\t\t\"hardware_version\": \"\",\n"\
"\t\t\t\t\t\t\"model\": \"###CAMERANAME###\"\n"\
"\t\t\t\t\t}\n"\
"\t\t\t\t}\n"\
"\t\t\t},\n"\
"\t\t\t\"flight_logging\": {\n"\
"\t\t\t\t\"flight_logging_items\": [\n";

const char* kUTMLoggingKeys =
"\t\t\t\t],\n"\
"\t\t\t\t\"flight_logging_keys\": [\n"\
"\t\t\t\t\t\"timestamp\", \"gps_lon\", \"gps_lat\", \"gps_altitude\", \"speed\", \"battery_voltage\"\n"\
"\t\t\t\t],\n"\
"\t\t\t\t\"altitude_system\": \"WGS84\",\n";

const char* kSKYLoggingKeys =
"\t\t\t\t],\n"\
"\t\t\t\t\"flight_logging_keys\": [\n"\
"\t\t\t\t\t\"timestamp\",\n"\
"\t\t\t\t\t\"aircraft_lon\",\n"\
"\t\t\t\t\t\"aircraft_lat\",\n"\
"\t\t\t\t\t\"aircraft_altitude\",\n"\
"\t\t\t\t\t\"aircraft_speed\",\n"\
"\t\t\t\t\t\"battery_voltage\"\n"\
"\t\t\t\t],\n";

const char* kUTMLoggingFooter =
"\t\t\t},\n"\
"\t\t\t\"file\": {\n"\
"\t\t\t\t\"logging_type\": \"GUTMA_DX_JSON\",\n"\
"\t\t\t\t\"filename\": \"###FILENAME###\",\n"\
"\t\t\t\t\"creation_dtg\": \"###FILEDATE###Z\"\n"\
"\t\t\t},\n"\
"\t\t\t\"message_type\": \"flight_logging_submission\"\n"\
"\t\t}\n"\
"\t}\n"\
"}\n";

const char* kSKYLoggingFooter =
"\t\t\t\t\"altitude_system\": \"WGS84\",\n"\
"\t\t\t\t\"logging_start_dtg\": \"###LOGSTART###Z\"\n"\
"\t\t\t},\n"\
"\t\t\t\"file\":\t{\n"\
"\t\t\t\t\"logging_type\": \"SKYWARD_GCS\",\n"\
"\t\t\t\t\"filename\": \"###FILENAME###\",\n"\
"\t\t\t\t\"creation_dtg\": \"###FILEDATE###Z\"\n"\
"\t\t\t},\n"\
"\t\t\t\"message_type\": \"flight_logging_submission\"\n"\
"\t\t}\n"\
"\t}\n"\
"}";

//-----------------------------------------------------------------------------
UTMConverter::UTMConverter(bool skyward)
    : _curTimeUSecs(0)
    , _startDTG(0)
    , _lastSpeed(0)
    , _lastBattery(0)
    , _gpsRawIntMessageAvailable(false)
    , _globalPositionIntMessageAvailable(false)
    , _mavlinkChannel(0)
    , _cancel(false)
    , _convertToSkyward(skyward)
{

}

//-----------------------------------------------------------------------------
UTMConverter::~UTMConverter()
{
    if(_mavlinkChannel) {
        qgcApp()->toolbox()->linkManager()->_freeMavlinkChannel(_mavlinkChannel);
        _mavlinkChannel = 0;
    }
}

//-----------------------------------------------------------------------------
void
UTMConverter::cancel()
{
    _cancel = true;
}

//-----------------------------------------------------------------------------
bool
UTMConverter::convertTelemetryFile(const QString& srcFilename, const QString& dstFilename)
{
    if(!_mavlinkChannel) {
        _mavlinkChannel = qgcApp()->toolbox()->linkManager()->_reserveMavlinkChannel();
    }
    if (_mavlinkChannel == 0) {
        qWarning() << "No mavlink channels available";
        return false;
    }
    //-- Open source Telemetry File
    _logFile.setFileName(srcFilename);
    if (!_logFile.open(QFile::ReadOnly)) {
        qWarning() << QString("Unable to open log file: '%1', error: %2").arg(srcFilename).arg(_logFile.errorString());
        return false;
    }
    //-- Create Destination UTM File
    QFile _utmLogFile(dstFilename);
    if (!_utmLogFile.open(QFile::WriteOnly)) {
        qWarning() << QString("Unable to create UTM file: '%1', error: %2").arg(dstFilename).arg(_utmLogFile.errorString());
        _logFile.close();
        return false;
    }
    //-- TODO: Make sure the time stamp is stored in UTC
    QByteArray timestamp = _logFile.read(TIMESTAMP_SIZE);
    _curTimeUSecs = _parseTimestamp(timestamp);
    //-- Parse log file
    while(!_cancel) {
        mavlink_message_t message;
        qint64 nextTimeUSecs = _readNextMavlinkMessage(message);
        if(!nextTimeUSecs || _cancel) {
            break;
        }
        _newMavlinkMessage(_curTimeUSecs, message);
        _curTimeUSecs = nextTimeUSecs;
    }
    //-- Write UTM File
    if(!_cancel && _logItems.size()) {
        //-- Header
        QString header;
        if(_convertToSkyward) {
            header = kSKYLoggingHeader;
            header.replace("###GIMBALFIRMWARE###", _gimbalVersion);
            header.replace("###FLIGHTSTART###", QDateTime::fromMSecsSinceEpoch(_startDTG / 1000).toString(Qt::ISODateWithMs));
            header.replace("###FLIGHTEND###", QDateTime::fromMSecsSinceEpoch(_curTimeUSecs / 1000).toString(Qt::ISODateWithMs));
        } else {
            header = kUTMLoggingHeader;
        }
        header.replace("###APFIRMWARE###", _apVersion);
        header.replace("###APSERIAL###", _apUID);
        header.replace("###DPVERSION###", qgcApp()->applicationVersion());
        header.replace("###CAMERANAME###", _cameraModel);
        header.replace("###CAMERAFIRMWARE###", _cameraVersion);
        _utmLogFile.write(header.toLocal8Bit());
        for(int i = 0; i < _logItems.size(); i++) {
            if(_cancel) {
                break;
            }
            QString line;
            if(_convertToSkyward) {
                QDateTime timeS = QDateTime::fromMSecsSinceEpoch(_logItems[i].timeStamp / 1000);
                line.sprintf("\t\t\t\t\t[\"%sZ\", \"%f\", \"%f\", \"%.3f\", \"%.3f\", \"%.3f\" ]",
                    timeS.toString(Qt::ISODateWithMs).toLocal8Bit().data(),
                    _logItems[i].lon,
                    _logItems[i].lat,
                    _logItems[i].alt,
                    _logItems[i].speed,
                    _logItems[i].battery);
            } else {
                line.sprintf("\t\t\t\t\t[%.3f, %f, %f, %.3f, %.3f, %.3f ]",
                    _logItems[i].time,
                    _logItems[i].lon,
                    _logItems[i].lat,
                    _logItems[i].alt,
                    _logItems[i].speed,
                    _logItems[i].battery);
            }
            if(i < _logItems.size() - 1) {
                line += ",\n";
            } else {
                line += "\n";
            }
            _utmLogFile.write(line.toLocal8Bit());
        }
        //-- Keys
        if(_convertToSkyward) {
            _utmLogFile.write(kSKYLoggingKeys);
        } else {
            _utmLogFile.write(kUTMLoggingKeys);
        }
        //-- Footer
        QDateTime dtg = QDateTime::fromMSecsSinceEpoch(_startDTG / 1000);
        QString logStart = dtg.toString(Qt::ISODateWithMs);
        if(!_convertToSkyward) {
            QString line = QString("\t\t\t\t\"logging_start_dtg\": \"%1Z\"\n").arg(logStart);
            _utmLogFile.write(line.toLocal8Bit());
        }
        QString footer;
        QFileInfo fi(dstFilename);
        if(_convertToSkyward) {
            footer = kSKYLoggingFooter;
            footer.replace("###LOGSTART###", logStart);
        } else {
            footer = kUTMLoggingFooter;
        }
        footer.replace("###FILENAME###", fi.baseName());
        footer.replace("###FILEDATE###", QDateTime::currentDateTime().toString(Qt::ISODateWithMs));
        _utmLogFile.write(footer.toLocal8Bit());
    }
    _utmLogFile.close();
    //-- If there was nothing, remove empty file
    if(!_logItems.size() || _cancel) {
        _utmLogFile.remove();
    }
    return true;
}

//-----------------------------------------------------------------------------
quint64
UTMConverter::_readNextMavlinkMessage(mavlink_message_t& message)
{
    char                nextByte;
    mavlink_status_t    status;
    while (_logFile.getChar(&nextByte) && !_cancel) { // Loop over every byte
        bool messageFound = mavlink_parse_char(_mavlinkChannel, nextByte, &message, &status);
        if (messageFound) {
            // Return the timestamp for the next message
            QByteArray rawTime = _logFile.read(TIMESTAMP_SIZE);
            return _parseTimestamp(rawTime);
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
quint64
UTMConverter::_parseTimestamp(const QByteArray& bytes)
{
    quint64 timestamp = qFromBigEndian(*((quint64*)(bytes.constData())));
    quint64 currentTimestamp = ((quint64)QDateTime::currentMSecsSinceEpoch()) * 1000;
    if (timestamp > currentTimestamp) {
        timestamp = qbswap(timestamp);
    }
    return timestamp;
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
    case MAVLINK_MSG_ID_BATTERY_STATUS:
        _handleBatteryStatus(message);
        break;
    case MAVLINK_MSG_ID_CAMERA_INFORMATION:
        _handleCameraInfo(message);
        break;
    case MAVLINK_MSG_ID_AUTOPILOT_VERSION:
        _handleAutopilotVersion(message);
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
        //-- Skip it if it's less than 250ms from last
        double curElapsed = (_curTimeUSecs - _startDTG) / 1000000.0;
        if(_logItems.size()) {
            if(curElapsed - _logItems[_logItems.size()-1].time < 0.25) {
                return;
            }
        }
        mavlink_gps_raw_int_t gpsRawInt;
        mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);
        if (gpsRawInt.fix_type >= GPS_FIX_TYPE_3D_FIX) {
            UTM_LogItem logItem;
            logItem.lon   = gpsRawInt.lat / (double)1E7;
            logItem.lat   = gpsRawInt.lat / (double)1E7;
            logItem.alt   = gpsRawInt.alt / 1000.0;
            logItem.time  = curElapsed;
            logItem.timeStamp = _curTimeUSecs;
            logItem.speed = _lastSpeed;
            logItem.battery = _lastBattery;
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
    //-- Skip it if it's less than 250ms from last
    double curElapsed = (_curTimeUSecs - _startDTG) / 1000000.0;
    if(_logItems.size()) {
        if(curElapsed - _logItems[_logItems.size()-1].time < 0.25) {
            return;
        }
    }
    mavlink_global_position_int_t globalPositionInt;
    mavlink_msg_global_position_int_decode(&message, &globalPositionInt);
    UTM_LogItem logItem;
    logItem.lon   = globalPositionInt.lon / (double)1E7;
    logItem.lat   = globalPositionInt.lat / (double)1E7;
    logItem.alt   = globalPositionInt.alt / 1000.0;
    logItem.time  = curElapsed;
    logItem.timeStamp = _curTimeUSecs;
    logItem.speed = _lastSpeed;
    logItem.battery = _lastBattery;
    if(_logItems.size()) {
        if(!_compareItem(_logItems[_logItems.size()-1], logItem)) {
            _logItems.append(logItem);
        }
    } else {
        _logItems.append(logItem);
    }
}

//-----------------------------------------------------------------------------
void
UTMConverter::_handleBatteryStatus(mavlink_message_t& message)
{
    mavlink_battery_status_t bat_status;
    mavlink_msg_battery_status_decode(&message, &bat_status);
    _lastBattery = 0;
    for (int i = 0; i < 10; i++) {
        if (bat_status.voltages[i] != UINT16_MAX) {
            _lastBattery += (double)bat_status.voltages[i] / 1000.0;
        }
    }
}

//-----------------------------------------------------------------------------
void
UTMConverter::_handleCameraInfo(mavlink_message_t& message)
{
    mavlink_camera_information_t info;
    mavlink_msg_camera_information_decode(&message, &info);
    _cameraModel  = (const char*)(void*)&info.model_name[0];
    char cntry = (info.firmware_version >> 24) & 0xFF;
    int  build = (info.firmware_version >> 16) & 0xFF;
    int  minor = (info.firmware_version >>  8) & 0xFF;
    int  major = info.firmware_version & 0xFF;
    _cameraVersion.sprintf("%d.%d.%d_%c", major, minor, build, cntry);
    qDebug() << "Camera:" << _cameraVersion;
}

//-----------------------------------------------------------------------------
void
UTMConverter::_handleAutopilotVersion(mavlink_message_t& message)
{
    mavlink_autopilot_version_t autopilotVersion;
    mavlink_msg_autopilot_version_decode(&message, &autopilotVersion);
    if (message.compid == MAV_COMP_ID_GIMBAL) {
        int major = (autopilotVersion.flight_sw_version >> (8 * 3)) & 0xFF;
        int minor = (autopilotVersion.flight_sw_version >> (8 * 2)) & 0xFF;
        int patch = (autopilotVersion.flight_sw_version >> (8 * 1)) & 0xFF;
        _gimbalVersion.sprintf("%d.%d.%d", major, minor, patch);
        qDebug() << "Gimbal:" << _gimbalVersion;
    } else {
        uint8_t* pUid = (uint8_t*)(void*)&autopilotVersion.uid;
        _apUID.sprintf("%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X",
            pUid[0] & 0xff,
            pUid[1] & 0xff,
            pUid[2] & 0xff,
            pUid[3] & 0xff,
            pUid[4] & 0xff,
            pUid[5] & 0xff,
            pUid[6] & 0xff,
            pUid[7] & 0xff);
        _apVersion.sprintf("%d.%d.%d",
            autopilotVersion.flight_custom_version[2],
            autopilotVersion.flight_custom_version[1],
            autopilotVersion.flight_custom_version[0]);
        qDebug() << "AP:" << _apVersion << _apUID;
    }
}
