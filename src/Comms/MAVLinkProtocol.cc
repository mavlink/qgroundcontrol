/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkProtocol.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "QGCTemporaryFile.h"
#include "SettingsManager.h"
#include "MavlinkSettings.h"
#include "AppSettings.h"
#include "QmlObjectListModel.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaType>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

QGC_LOGGING_CATEGORY(MAVLinkProtocolLog, "qgc.comms.mavlinkprotocol")

Q_APPLICATION_STATIC(MAVLinkProtocol, _mavlinkProtocolInstance);

MAVLinkProtocol::MAVLinkProtocol(QObject *parent)
    : QObject(parent)
    , _tempLogFile(new QGCTemporaryFile(QStringLiteral("%2.%3").arg(_tempLogFileTemplate, _logFileExtension), this))
{
    // qCDebug(MAVLinkProtocolLog) << Q_FUNC_INFO << this;
}

MAVLinkProtocol::~MAVLinkProtocol()
{
    _closeLogFile();

    // qCDebug(MAVLinkProtocolLog) << Q_FUNC_INFO << this;
}

MAVLinkProtocol *MAVLinkProtocol::instance()
{
    return _mavlinkProtocolInstance();
}

void MAVLinkProtocol::init()
{
    if (_initialized) {
        return;
    }

    (void) memset(_firstMessage, 1, sizeof(_firstMessage));

    (void) connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleRemoved, this, &MAVLinkProtocol::_vehicleCountChanged);

    _initialized = true;
}

void MAVLinkProtocol::setVersion(unsigned version)
{
    const QList<SharedLinkInterfacePtr> sharedLinks = LinkManager::instance()->links();
    for (const SharedLinkInterfacePtr &interface : sharedLinks) {
        mavlink_set_proto_version(interface.get()->mavlinkChannel(), version / 100);
    }

    _currentVersion = version;
}

void MAVLinkProtocol::resetMetadataForLink(LinkInterface *link)
{
    const uint8_t channel = link->mavlinkChannel();
    _totalReceiveCounter[channel] = 0;
    _totalLossCounter[channel] = 0;
    _runningLossPercent[channel] = 0.f;
    for (int i = 0; i < 256; i++) {
        _firstMessage[channel][i] = 1;
    }

    link->setDecodedFirstMavlinkPacket(false);
}

void MAVLinkProtocol::logSentBytes(const LinkInterface *link, const QByteArray &data)
{
    Q_UNUSED(link);

    if (_logSuspendError || _logSuspendReplay || !_tempLogFile->isOpen()) {
        return;
    }

    const quint64 time = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch() * 1000);
    uint8_t bytes_time[sizeof(quint64)]{};
    qToBigEndian(time, bytes_time);

    QByteArray logData = data;
    QByteArray timeData = QByteArray::fromRawData(reinterpret_cast<const char*>(bytes_time), sizeof(bytes_time));
    (void) logData.prepend(timeData);
    if (_tempLogFile->write(logData) != logData.length()) {
        const QString message = QStringLiteral("MAVLink Logging failed. Could not write to file %1, logging disabled.").arg(_tempLogFile->fileName());
        qgcApp()->showAppMessage(message, getName());
        _stopLogging();
        _logSuspendError = true;
    }
}

void MAVLinkProtocol::receiveBytes(LinkInterface *link, const QByteArray &data)
{
    const SharedLinkInterfacePtr linkPtr = LinkManager::instance()->sharedLinkInterfacePointerForLink(link);
    if (!linkPtr) {
        qCDebug(MAVLinkProtocolLog) << "receiveBytes: link gone!" << data.size() << "bytes arrived too late";
        return;
    }

    for (const uint8_t &byte: data) {
        const uint8_t mavlinkChannel = link->mavlinkChannel();
        mavlink_message_t message{};
        mavlink_status_t status{};

        if (mavlink_parse_char(mavlinkChannel, byte, &message, &status) != MAVLINK_FRAMING_OK) {
            continue;
        }

        _updateVersion(link, mavlinkChannel);
        _updateCounters(mavlinkChannel, message);
        if (!linkPtr->linkConfiguration()->isForwarding()) {
            _forward(message);
            _forwardSupport(message);
        }
        _logData(link, message);

        if (!_updateStatus(link, linkPtr, mavlinkChannel, message)) {
            break;
        }
    }
}

void MAVLinkProtocol::_updateVersion(LinkInterface *link, uint8_t mavlinkChannel)
{
    if (link->decodedFirstMavlinkPacket()) {
        return;
    }

    link->setDecodedFirstMavlinkPacket(true);
    const mavlink_status_t *const mavlinkStatus = mavlink_get_channel_status(mavlinkChannel);

    if (mavlinkStatus->flags & MAVLINK_STATUS_FLAG_IN_MAVLINK1) {
        return;
    }

    if (mavlink_get_proto_version(mavlinkChannel) == 1) {
        qCDebug(MAVLinkProtocolLog) << "Switching outbound to mavlink 2.0 due to incoming mavlink 2.0 packet:" << mavlinkChannel;
        setVersion(200);
    }
}

void MAVLinkProtocol::_updateCounters(uint8_t mavlinkChannel, const mavlink_message_t &message)
{
    uint8_t lastSeq = _lastIndex[message.sysid][message.compid];
    uint8_t expectedSeq = lastSeq + 1;
    _totalReceiveCounter[mavlinkChannel]++;
    if (_firstMessage[message.sysid][message.compid] != 0) {
        _firstMessage[message.sysid][message.compid] = 0;
        lastSeq = message.seq;
        expectedSeq = message.seq;
    }

    if (message.seq != expectedSeq) {
        uint64_t lostMessages = message.seq;
        if (message.seq < expectedSeq) {
            lostMessages += 255;
        }
        lostMessages -= expectedSeq;
        _totalLossCounter[mavlinkChannel] += lostMessages;
    }

    _lastIndex[message.sysid][message.compid] = message.seq;

    const uint64_t totalSent = _totalReceiveCounter[mavlinkChannel] + _totalLossCounter[mavlinkChannel];
    float receiveLossPercent = static_cast<float>(static_cast<double>(_totalLossCounter[mavlinkChannel]) / static_cast<double>(totalSent));
    receiveLossPercent *= 100.0f;
    receiveLossPercent *= 0.5f;
    receiveLossPercent += (_runningLossPercent[mavlinkChannel] * 0.5f);
    _runningLossPercent[mavlinkChannel] = receiveLossPercent;
}

void MAVLinkProtocol::_forward(const mavlink_message_t &message)
{
    if (message.msgid == MAVLINK_MSG_ID_SETUP_SIGNING) {
        return;
    }

    if (!SettingsManager::instance()->mavlinkSettings()->forwardMavlink()->rawValue().toBool()) {
        return;
    }

    SharedLinkInterfacePtr forwardingLink = LinkManager::instance()->mavlinkForwardingLink();
    if (!forwardingLink) {
        return;
    }

    uint8_t buf[MAVLINK_MAX_PACKET_LEN]{};
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &message);
    (void) forwardingLink->writeBytesThreadSafe(reinterpret_cast<const char*>(buf), len);
}

void MAVLinkProtocol::_forwardSupport(const mavlink_message_t &message)
{
    if (message.msgid == MAVLINK_MSG_ID_SETUP_SIGNING) {
        return;
    }

    if (!LinkManager::instance()->mavlinkSupportForwardingEnabled()) {
        return;
    }

    SharedLinkInterfacePtr forwardingSupportLink = LinkManager::instance()->mavlinkForwardingSupportLink();
    if (!forwardingSupportLink) {
        return;
    }

    uint8_t buf[MAVLINK_MAX_PACKET_LEN]{};
    const uint16_t len = mavlink_msg_to_send_buffer(buf, &message);
    (void) forwardingSupportLink->writeBytesThreadSafe(reinterpret_cast<const char*>(buf), len);
}

void MAVLinkProtocol::_logData(LinkInterface *link, const mavlink_message_t &message)
{
    if (!_logSuspendError && !_logSuspendReplay && _tempLogFile->isOpen()) {
        const quint64 timestamp = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch() * 1000);
        uint8_t buf[MAVLINK_MAX_PACKET_LEN + sizeof(timestamp)]{};
        qToBigEndian(timestamp, buf);

        const qsizetype len = mavlink_msg_to_send_buffer(buf + sizeof(timestamp), &message) + sizeof(timestamp);
        const QByteArray log_data(reinterpret_cast<const char*>(buf), len);
        if (_tempLogFile->write(log_data) != len) {
            const QString message = QStringLiteral("MAVLink Logging failed. Could not write to file %1, logging disabled.").arg(_tempLogFile->fileName());
            qgcApp()->showAppMessage(message, getName());
            _stopLogging();
            _logSuspendError = true;
        }

        if ((message.msgid == MAVLINK_MSG_ID_HEARTBEAT) && !_vehicleWasArmed) {
            if (mavlink_msg_heartbeat_get_base_mode(&message) & MAV_MODE_FLAG_DECODE_POSITION_SAFETY) {
                _vehicleWasArmed = true;
            }
        }
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HEARTBEAT: {
        _startLogging();
        mavlink_heartbeat_t heartbeat{};
        mavlink_msg_heartbeat_decode(&message, &heartbeat);
        emit vehicleHeartbeatInfo(link, message.sysid, message.compid, heartbeat.autopilot, heartbeat.type);
        break;
    }
    case MAVLINK_MSG_ID_HIGH_LATENCY: {
        _startLogging();
        mavlink_high_latency_t highLatency{};
        mavlink_msg_high_latency_decode(&message, &highLatency);
        // HIGH_LATENCY does not provide autopilot or type information, generic is our safest bet
        emit vehicleHeartbeatInfo(link, message.sysid, message.compid, MAV_AUTOPILOT_GENERIC, MAV_TYPE_GENERIC);
        break;
    }
    case MAVLINK_MSG_ID_HIGH_LATENCY2: {
        _startLogging();
        mavlink_high_latency2_t highLatency2{};
        mavlink_msg_high_latency2_decode(&message, &highLatency2);
        emit vehicleHeartbeatInfo(link, message.sysid, message.compid, highLatency2.autopilot, highLatency2.type);
        break;
    }
    default:
        break;
    }
}

bool MAVLinkProtocol::_updateStatus(LinkInterface *link, const SharedLinkInterfacePtr linkPtr, uint8_t mavlinkChannel, const mavlink_message_t &message)
{
    if ((_totalReceiveCounter[mavlinkChannel] % 31) == 0) {
        const uint64_t totalSent = _totalReceiveCounter[mavlinkChannel] + _totalLossCounter[mavlinkChannel];
        emit mavlinkMessageStatus(message.sysid, totalSent, _totalReceiveCounter[mavlinkChannel], _totalLossCounter[mavlinkChannel], _runningLossPercent[mavlinkChannel]);
    }

    emit messageReceived(link, message);

    if (linkPtr.use_count() == 1) {
        return false;
    }

    return true;
}

bool MAVLinkProtocol::_closeLogFile()
{
    if (!_tempLogFile->isOpen()) {
        return false;
    }

    if (_tempLogFile->size() == 0) {
        (void) _tempLogFile->remove();
        return false;
    }

    (void) _tempLogFile->flush();
    _tempLogFile->close();
    return true;
}

void MAVLinkProtocol::_startLogging()
{
    if (qgcApp()->runningUnitTests()) {
        return;
    }

    AppSettings *const appSettings = SettingsManager::instance()->appSettings();
    if (appSettings->disableAllPersistence()->rawValue().toBool()) {
        return;
    }

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    if (!SettingsManager::instance()->mavlinkSettings()->telemetrySave()->rawValue().toBool()) {
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
        qgcApp()->showAppMessage(message, getName());
        _closeLogFile();
        _logSuspendError = true;
        return;
    }

    qCDebug(MAVLinkProtocolLog) << "Temp log" << _tempLogFile->fileName();
    (void) _checkTelemetrySavePath();

    _logSuspendError = false;
}

void MAVLinkProtocol::_stopLogging()
{
    if (_tempLogFile->isOpen() && _closeLogFile()) {
        auto appSettings = SettingsManager::instance()->appSettings();
        auto mavlinkSettings = SettingsManager::instance()->mavlinkSettings();
        if ((_vehicleWasArmed || mavlinkSettings->telemetrySaveNotArmed()->rawValue().toBool()) && 
                mavlinkSettings->telemetrySave()->rawValue().toBool() && 
                !appSettings->disableAllPersistence()->rawValue().toBool()) {
            _saveTelemetryLog(_tempLogFile->fileName());
        } else {
            (void) QFile::remove(_tempLogFile->fileName());
        }
    }

    _vehicleWasArmed = false;
}

void MAVLinkProtocol::checkForLostLogFiles()
{
    static const QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    static const QString filter(QStringLiteral("*.%1").arg(_logFileExtension));
    static const QStringList filterList(filter);

    const QFileInfoList fileInfoList = tempDir.entryInfoList(filterList, QDir::Files);
    qCDebug(MAVLinkProtocolLog) << "Orphaned log file count" << fileInfoList.count();

    for (const QFileInfo &fileInfo: fileInfoList) {
        qCDebug(MAVLinkProtocolLog) << "Orphaned log file" << fileInfo.filePath();
        if (fileInfo.size() == 0) {
            (void) QFile::remove(fileInfo.filePath());
            continue;
        }
        _saveTelemetryLog(fileInfo.filePath());
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

void MAVLinkProtocol::_saveTelemetryLog(const QString &tempLogfile)
{
    if (_checkTelemetrySavePath()) {
        const QString saveDirPath = SettingsManager::instance()->appSettings()->telemetrySavePath();
        const QDir saveDir(saveDirPath);

        const QString nameFormat("%1%2.%3");
        const QString dtFormat("yyyy-MM-dd hh-mm-ss");

        int tryIndex = 1;
        QString saveFileName = nameFormat.arg(QDateTime::currentDateTime().toString(dtFormat), QStringLiteral(""), AppSettings::telemetryFileExtension);
        while (saveDir.exists(saveFileName)) {
            saveFileName = nameFormat.arg(QDateTime::currentDateTime().toString(dtFormat), QStringLiteral(".%1").arg(tryIndex++), AppSettings::telemetryFileExtension);
        }

        const QString saveFilePath = saveDir.absoluteFilePath(saveFileName);
        QFile tempFile(tempLogfile);
        if (!tempFile.copy(saveFilePath)) {
            const QString error = tr("Unable to save telemetry log. Error copying telemetry to '%1': '%2'.").arg(saveFilePath, tempFile.errorString());
            qgcApp()->showAppMessage(error);
        }
    }

    (void) QFile::remove(tempLogfile);
}

bool MAVLinkProtocol::_checkTelemetrySavePath()
{
    const QString saveDirPath = SettingsManager::instance()->appSettings()->telemetrySavePath();
    if (saveDirPath.isEmpty()) {
        const QString error = tr("Unable to save telemetry log. Application save directory is not set.");
        qgcApp()->showAppMessage(error);
        return false;
    }

    const QDir saveDir(saveDirPath);
    if (!saveDir.exists()) {
        const QString error = tr("Unable to save telemetry log. Telemetry save directory \"%1\" does not exist.").arg(saveDirPath);
        qgcApp()->showAppMessage(error);
        return false;
    }

    return true;
}

void MAVLinkProtocol::_vehicleCountChanged()
{
    if (MultiVehicleManager::instance()->vehicles()->count() == 0) {
        _stopLogging();
    }
}

int MAVLinkProtocol::getSystemId() const 
{ 
    return SettingsManager::instance()->mavlinkSettings()->gcsMavlinkSystemID()->rawValue().toInt(); 
}
