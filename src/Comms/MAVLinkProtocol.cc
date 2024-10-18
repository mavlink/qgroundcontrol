/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkProtocol.h"
#include "LinkInterface.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCTemporaryFile.h"
#include "QGCToolbox.h"
#include "SettingsManager.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaType>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

#include <QtQml/QtQml>

QGC_LOGGING_CATEGORY(MAVLinkProtocolLog, "qgc.comms.mavlinkprotocol")

Q_APPLICATION_STATIC(MAVLinkProtocol, _mavlinkProtocol);

MAVLinkProtocol::MAVLinkProtocol(QObject *parent)
    : QObject(parent)
    , _tempLogFile(new QGCTemporaryFile(QStringLiteral("%2.%3").arg(_tempLogFileTemplate, _logFileExtension)))
{
    // qCDebug(MAVLinkProtocolLog) << Q_FUNC_INFO << this;

    (void) memset(_firstMessage, 1, sizeof(_firstMessage));

    (void) connect(this, &MAVLinkProtocol::protocolStatusMessage, qgcApp(), &QGCApplication::criticalMessageBoxOnMainThread);
    (void) connect(this, &MAVLinkProtocol::saveTelemetryLog, qgcApp(), &QGCApplication::saveTelemetryLogOnMainThread);
    (void) connect(this, &MAVLinkProtocol::checkTelemetrySavePath, qgcApp(), &QGCApplication::checkTelemetrySavePathOnMainThread);

    (void) connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleAdded, this, &MAVLinkProtocol::_vehicleCountChanged);
    (void) connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::vehicleRemoved, this, &MAVLinkProtocol::_vehicleCountChanged);

    loadSettings();
}

MAVLinkProtocol::~MAVLinkProtocol()
{
    storeSettings();
    _closeLogFile();

    // qCDebug(MAVLinkProtocolLog) << Q_FUNC_INFO << this;
}

MAVLinkProtocol *MAVLinkProtocol::instance()
{
    return _mavlinkProtocol();
}

void MAVLinkProtocol::setVersion(uint16_t version)
{
    const QList<SharedLinkInterfacePtr> sharedLinks = qgcApp()->toolbox()->linkManager()->links();
    for (const SharedLinkInterfacePtr &interface : sharedLinks) {
        mavlink_set_proto_version(interface.get()->mavlinkChannel(), version);
    }

    _currentVersion = version;
}

void MAVLinkProtocol::loadSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAVLINK_PROTOCOL");
    enableVersionCheck(settings.value("VERSION_CHECK_ENABLED", versionCheckEnabled()).toBool());

    bool ok = false;
    const uint temp = settings.value("GCS_SYSTEM_ID", getSystemId()).toUInt(&ok);
    if (ok && (temp > 0) && (temp < 256)) {
        setSystemId(temp);
    }
    settings.endGroup();
}

void MAVLinkProtocol::storeSettings()
{
    QSettings settings;
    settings.beginGroup("QGC_MAVLINK_PROTOCOL");

    settings.setValue("VERSION_CHECK_ENABLED", versionCheckEnabled());
    settings.setValue("GCS_SYSTEM_ID", getSystemId());

    settings.endGroup();
}

void MAVLinkProtocol::resetMetadataForLink(LinkInterface *link)
{
    const uint8_t channel = link->mavlinkChannel();
    _totalReceiveCounter[channel] = 0;
    _totalLossCounter[channel] = 0;
    _runningLossPercent[channel] = 0.f;
    for (uint16_t i = 0; i < 256; i++) {
        _firstMessage[channel][i] = 1;
    }
    link->setDecodedFirstMavlinkPacket(false);
}

void MAVLinkProtocol::logSentBytes(LinkInterface *link, const QByteArray &data)
{
    Q_UNUSED(link);

    uint8_t bytes_time[sizeof(qint64)];
    if (!_logSuspendError && !_logSuspendReplay && _tempLogFile->isOpen()) {
        const qint64 time = QDateTime::currentMSecsSinceEpoch() * 1000;
        qToBigEndian(time, bytes_time);
        QByteArray logData = QByteArray();
        (void) logData.insert(0, QByteArray(reinterpret_cast<const char*>(bytes_time), sizeof(bytes_time)));

        const qsizetype len = logData.length();
        if (_tempLogFile->write(logData) != logData.length()) {
            const QString message = QStringLiteral("MAVLink Logging failed. Could not write to file %1, logging disabled.").arg(_tempLogFile->fileName());
            emit protocolStatusMessage(getName(), message);
            _stopLogging();
            _logSuspendError = true;
        }
    }
}

void MAVLinkProtocol::receiveBytes(LinkInterface *link, const QByteArray &data)
{
    const SharedLinkInterfacePtr linkPtr = qgcApp()->toolbox()->linkManager()->sharedLinkInterfacePointerForLink(link);
    if (!linkPtr) {
        qCDebug(MAVLinkProtocolLog) << "receiveBytes: link gone!" << data.size() << " bytes arrived too late";
        return;
    }

    const uint8_t mavlinkChannel = link->mavlinkChannel();
    for (const uint8_t &byte: data) {
        if (mavlink_parse_char(mavlinkChannel, byte, &_message, &_status) == MAVLINK_FRAMING_OK) {
            _updateVersion(link, mavlinkChannel);
            _updateCounters(mavlinkChannel);
            _forward();
            _forwardSupport();
            _logData(link);
            if (!_updateStatus(link, linkPtr, mavlinkChannel)) {
                break;
            }
        }
    }
}

bool MAVLinkProtocol::_updateStatus(LinkInterface *link, SharedLinkInterfacePtr linkPtr, uint8_t mavlinkChannel)
{
    if ((_totalReceiveCounter[mavlinkChannel] % 32) == 0) {
        const uint64_t totalSent = _totalReceiveCounter[mavlinkChannel] + _totalLossCounter[mavlinkChannel];
        emit mavlinkMessageStatus(_message.sysid, totalSent, _totalReceiveCounter[mavlinkChannel], _totalLossCounter[mavlinkChannel], _runningLossPercent[mavlinkChannel]);
    }

    emit messageReceived(link, _message);

    if (linkPtr.use_count() == 1) {
        return false;
    }

    (void) memset(&_status, 0, sizeof(_status));
    (void) memset(&_message, 0, sizeof(_message));

    return true;
}

void MAVLinkProtocol::_updateVersion(LinkInterface *link, uint8_t mavlinkChannel)
{
    if (!link->decodedFirstMavlinkPacket()) {
        link->setDecodedFirstMavlinkPacket(true);
        mavlink_status_t* const mavlinkStatus = mavlink_get_channel_status(mavlinkChannel);
        if ((!(mavlinkStatus->flags & MAVLINK_STATUS_FLAG_IN_MAVLINK1)) && (mavlinkStatus->flags & MAVLINK_STATUS_FLAG_OUT_MAVLINK1)) {
            qCDebug(MAVLinkProtocolLog) << QStringLiteral("Switching outbound to mavlink 2.0 due to incoming mavlink 2.0 packet:") << mavlinkChannel;
            mavlinkStatus->flags &= ~MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
            setVersion(2);
        }
    }
}

void MAVLinkProtocol::_updateCounters(uint8_t mavlinkChannel)
{
    uint8_t lastSeq = _lastIndex[_message.sysid][_message.compid];
    uint8_t expectedSeq = lastSeq + 1;
    _totalReceiveCounter[mavlinkChannel]++;
    if (_firstMessage[_message.sysid][_message.compid] != 0) {
        _firstMessage[_message.sysid][_message.compid] = 0;
        lastSeq = _message.seq;
        expectedSeq = _message.seq;
    }

    if (_message.seq != expectedSeq)
    {
        uint64_t lostMessages = _message.seq;
        if (lostMessages < expectedSeq) {
            lostMessages += 255;
        }
        lostMessages -= expectedSeq;
        _totalLossCounter[mavlinkChannel] += lostMessages;
    }

    _lastIndex[_message.sysid][_message.compid] = _message.seq;
    const uint64_t totalSent = _totalReceiveCounter[mavlinkChannel] + _totalLossCounter[mavlinkChannel];
    float receiveLossPercent = static_cast<float>(static_cast<double>(_totalLossCounter[mavlinkChannel]) / static_cast<double>(totalSent));
    receiveLossPercent *= 100.0f;
    receiveLossPercent *= 0.5f;
    receiveLossPercent += (_runningLossPercent[mavlinkChannel] * 0.5f);
    _runningLossPercent[mavlinkChannel] = receiveLossPercent;
}

void MAVLinkProtocol::_forward()
{
    bool forwardingEnabled = qgcApp()->toolbox()->settingsManager()->appSettings()->forwardMavlink()->rawValue().toBool();
    if (_message.msgid == MAVLINK_MSG_ID_SETUP_SIGNING) {
        forwardingEnabled = false;
    }
    if (forwardingEnabled) {
        SharedLinkInterfacePtr forwardingLink = qgcApp()->toolbox()->linkManager()->mavlinkForwardingLink();
        if (forwardingLink) {
            uint8_t buf[MAVLINK_MAX_PACKET_LEN];
            const uint16_t len = mavlink_msg_to_send_buffer(buf, &_message);
            forwardingLink->writeBytesThreadSafe(reinterpret_cast<const char*>(buf), len);
        }
    }
}

void MAVLinkProtocol::_forwardSupport()
{
    LinkManager *linkManager = qgcApp()->toolbox()->linkManager();
    bool forwardingSupportEnabled = linkManager->mavlinkSupportForwardingEnabled();
    if (_message.msgid == MAVLINK_MSG_ID_SETUP_SIGNING) {
        forwardingSupportEnabled = false;
    }
    if (forwardingSupportEnabled) {
        SharedLinkInterfacePtr forwardingSupportLink = linkManager->mavlinkForwardingSupportLink();
        if (forwardingSupportLink) {
            uint8_t buf[MAVLINK_MAX_PACKET_LEN];
            const uint16_t len = mavlink_msg_to_send_buffer(buf, &_message);
            forwardingSupportLink->writeBytesThreadSafe(reinterpret_cast<const char*>(buf), len);
        }
    }
}

void MAVLinkProtocol::_logData(LinkInterface *link)
{
    if (!_logSuspendError && !_logSuspendReplay && _tempLogFile->isOpen()) {
        const qint64 timestamp = QDateTime::currentMSecsSinceEpoch() * 1000;
        uint8_t buf[MAVLINK_MAX_PACKET_LEN + sizeof(timestamp)];
        qToBigEndian(timestamp, buf);

        const qsizetype len = mavlink_msg_to_send_buffer(buf + sizeof(timestamp), &_message) + sizeof(timestamp);
        const QByteArray log_data(reinterpret_cast<const char*>(buf), len);
        if (_tempLogFile->write(log_data) != len) {
            const QString message = QStringLiteral("MAVLink Logging failed. Could not write to file %1, logging disabled.").arg(_tempLogFile->fileName());
            emit protocolStatusMessage(getName(), message);
            _stopLogging();
            _logSuspendError = true;
        }

        if (!_vehicleWasArmed && (_message.msgid == MAVLINK_MSG_ID_HEARTBEAT)) {
            mavlink_heartbeat_t state;
            mavlink_msg_heartbeat_decode(&_message, &state);
            if (state.base_mode & MAV_MODE_FLAG_DECODE_POSITION_SAFETY) {
                _vehicleWasArmed = true;
            }
        }
    }

    switch (_message.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT:
        _startLogging();
        mavlink_heartbeat_t heartbeat;
        mavlink_msg_heartbeat_decode(&_message, &heartbeat);
        emit vehicleHeartbeatInfo(link, _message.sysid, _message.compid, static_cast<MAV_AUTOPILOT>(heartbeat.autopilot), static_cast<MAV_TYPE>(heartbeat.type));
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _startLogging();
        mavlink_high_latency_t highLatency;
        mavlink_msg_high_latency_decode(&_message, &highLatency);
        // HIGH_LATENCY does not provide autopilot or type information, generic is our safest bet
        emit vehicleHeartbeatInfo(link, _message.sysid, _message.compid, MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
        break;
    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _startLogging();
        mavlink_high_latency2_t highLatency2;
        mavlink_msg_high_latency2_decode(&_message, &highLatency2);
        emit vehicleHeartbeatInfo(link, _message.sysid, _message.compid, static_cast<MAV_AUTOPILOT>(highLatency2.autopilot), static_cast<MAV_TYPE>(highLatency2.type));
        break;
    default:
        break;
    }
}

void MAVLinkProtocol::setSystemId(uint8_t id)
{
    if (id != _systemId) {
        _systemId = id;
        emit systemIdChanged(_systemId);
    }
}

void MAVLinkProtocol::enableVersionCheck(bool enabled)
{
    if (enabled != _enableVersionCheck) {
        _enableVersionCheck = enabled;
        emit versionCheckChanged(enabled);
    }
}

void MAVLinkProtocol::_vehicleCountChanged()
{
    if (qgcApp()->toolbox()->multiVehicleManager()->vehicles()->count() == 0) {
        _stopLogging();
        // _radioVersionMismatchCount = 0;
    }
}

bool MAVLinkProtocol::_closeLogFile()
{
    if (!_tempLogFile->isOpen()) {
        return false;
    }

    if (_tempLogFile->size() == 0) {
        _tempLogFile->remove();
        return false;
    } else {
        _tempLogFile->flush();
        _tempLogFile->close();
        return true;
    }
}

void MAVLinkProtocol::_startLogging()
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }

    AppSettings* const appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    if (appSettings->disableAllPersistence()->rawValue().toBool()) {
        return;
    }

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    if (!appSettings->telemetrySave()->rawValue().toBool()) {
        return;
    }
#endif

    if (_tempLogFile->isOpen()) {
        return;
    }

    if (_logSuspendReplay) {
        return;
    }

    if (!_tempLogFile->open()) {
        const QString message = QStringLiteral("Opening Flight Data file for writing failed. Unable to write to %1. Please choose a different file location.").arg(_tempLogFile->fileName());
        emit protocolStatusMessage(getName(), message);
        _closeLogFile();
        _logSuspendError = true;
        return;
    }

    qCDebug(MAVLinkProtocolLog) << "Temp log" << _tempLogFile->fileName();
    emit checkTelemetrySavePath();

    _logSuspendError = false;
}

void MAVLinkProtocol::_stopLogging()
{
    if (_tempLogFile->isOpen()) {
        if (_closeLogFile()) {
            AppSettings* const appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
            if ((_vehicleWasArmed || appSettings->telemetrySaveNotArmed()->rawValue().toBool()) &&
                appSettings->telemetrySave()->rawValue().toBool() &&
                !appSettings->disableAllPersistence()->rawValue().toBool()) {
                emit saveTelemetryLog(_tempLogFile->fileName());
            } else {
                (void) QFile::remove(_tempLogFile->fileName());
            }
        }
    }

    _vehicleWasArmed = false;
}

void MAVLinkProtocol::checkForLostLogFiles()
{
    static const QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    static const QString filter(QStringLiteral("*.%1").arg(_logFileExtension));
    const QFileInfoList fileInfoList = tempDir.entryInfoList(QStringList(filter), QDir::Files);
    qCDebug(MAVLinkProtocolLog) << "Orphaned log file count" << fileInfoList.count();

    for(const QFileInfo &fileInfo: fileInfoList) {
        qCDebug(MAVLinkProtocolLog) << "Orphaned log file" << fileInfo.filePath();
        if (fileInfo.size() == 0) {
            (void) QFile::remove(fileInfo.filePath());
            continue;
        }
        emit saveTelemetryLog(fileInfo.filePath());
    }
}

void MAVLinkProtocol::deleteTempLogFiles()
{
    static const QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    static const QString filter(QStringLiteral("*.%1").arg(_logFileExtension));
    const QFileInfoList fileInfoList = tempDir.entryInfoList(QStringList(filter), QDir::Files);
    qCDebug(MAVLinkProtocolLog) << "Temp log file count" << fileInfoList.count();

    for (const QFileInfo &fileInfo: fileInfoList) {
        qCDebug(MAVLinkProtocolLog) << "Temp log file" << fileInfo.filePath();
        (void) QFile::remove(fileInfo.filePath());
    }
}
