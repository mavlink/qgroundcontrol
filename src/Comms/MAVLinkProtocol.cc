#include "MAVLinkProtocol.h"

#include <QtCore/QApplicationStatic>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMetaType>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <cstring>

#include "AppMessages.h"
#include "AppSettings.h"
#include "LinkManager.h"
#include "MAVLinkLib.h"
#include "LinkInterface.h"
#include "MAVLinkSigning.h"
#include "SigningController.h"
#include "MavlinkSettings.h"
#include "MultiVehicleManager.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(MAVLinkProtocolLog, "Comms.MAVLinkProtocol")

Q_APPLICATION_STATIC(MAVLinkProtocol, _mavlinkProtocolInstance);

MAVLinkProtocol::MAVLinkProtocol(QObject* parent)
    : QObject(parent)
    , _logFile(new QFile(this))
{
    // Bound memory for arbitrary sender IDs, retaining the most recently active streams.
    for (auto& sequences : _lastIndex) {
        sequences.setMaxCost(4096);
    }
    qCDebug(MAVLinkProtocolLog) << this;
}

MAVLinkProtocol::~MAVLinkProtocol()
{
    _closeLogFile();

    qCDebug(MAVLinkProtocolLog) << this;
}

MAVLinkProtocol* MAVLinkProtocol::instance()
{
    return _mavlinkProtocolInstance();
}

void MAVLinkProtocol::init()
{
    if (_initialized) {
        return;
    }

    (void)connect(MultiVehicleManager::instance(), &MultiVehicleManager::vehicleRemoved, this,
                  &MAVLinkProtocol::_vehicleCountChanged);

    _initialized = true;
}

void MAVLinkProtocol::resetMetadataForLink(LinkInterface* link)
{
    const uint8_t channel = link->mavlinkChannel();
    _totalReceiveCounter[channel] = 0;
    _totalLossCounter[channel] = 0;
    _runningLossPercent[channel] = 0.f;

    resetSequenceTracking(link);
    link->setDecodedFirstMavlinkPacket(false);
}

void MAVLinkProtocol::resetSequenceTracking(LinkInterface* link)
{
    // Clear per-(sysid,compid) sequence state so next packet isn't counted as a gap.
    const uint8_t channel = link->mavlinkChannel();
    _lastIndex[channel].clear();
}

void MAVLinkProtocol::logSentBytes(const LinkInterface* link, const QByteArray& data)
{
    Q_UNUSED(link);

    if (_logSuspendError || _logSuspendReplay || !_logFile->isOpen()) {
        return;
    }

    const quint64 time = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch() * 1000);
    uint8_t bytes_time[sizeof(quint64)]{};
    qToBigEndian(time, bytes_time);

    QByteArray logData = data;
    QByteArray timeData = QByteArray::fromRawData(reinterpret_cast<const char*>(bytes_time), sizeof(bytes_time));
    (void)logData.prepend(timeData);
    if (_logFile->write(logData) != logData.length()) {
        const QString message = QStringLiteral("MAVLink Logging failed. Could not write to file %1, logging disabled.")
                                    .arg(_logFile->fileName());
        QGC::showAppMessage(message, getName());
        _stopLogging();
        _logSuspendError = true;
    } else if (_logKeepCheckPending) {
        _promoteLogFileIfKept();
    }
}

void MAVLinkProtocol::receiveBytes(LinkInterface* link, const QByteArray& data)
{
    const SharedLinkInterfacePtr linkPtr = LinkManager::instance()->sharedLinkInterfacePointerForLink(link);
    if (!linkPtr) {
        qCDebug(MAVLinkProtocolLog) << "receiveBytes: link gone!" << data.size() << "bytes arrived too late";
        return;
    }

    for (uint8_t byte : data) {
        const uint8_t mavlinkChannel = link->mavlinkChannel();
        mavlink_message_t message{};
        mavlink_status_t status{};

        const uint8_t framing = mavlink_parse_char(mavlinkChannel, byte, &message, &status);
        if (framing == MAVLINK_FRAMING_OK || framing == MAVLINK_FRAMING_BAD_SIGNATURE) {
            if (SigningController* const sigCtrl = link->signing()) {
                // Auto-detected key: reset sequence tracking so the key-install gap isn't counted as loss.
                if (sigCtrl->processFrame(framing == MAVLINK_FRAMING_OK, message)) {
                    resetSequenceTracking(link);
                }
            }
        }
        if (framing != MAVLINK_FRAMING_OK) {
            continue;
        }

        if (message.msgid == MAVLINK_MSG_ID_RADIO_STATUS) {
            link->reportRadioStatusReceived();
        }

        // v1/v2 share per-(sysid,compid) sequence counters; counting v1 makes every v2 appear lost. Skip v1 non-heartbeats.
        // RADIO_STATUS is exempt: SiK radios always frame it as v1, so it is processed and never triggers the v1 warning.
        const bool isV1 = (status.flags & MAVLINK_STATUS_FLAG_IN_MAVLINK1);
        if (isV1 && (message.msgid != MAVLINK_MSG_ID_HEARTBEAT) && (message.msgid != MAVLINK_MSG_ID_RADIO_STATUS)) {
            link->reportMavlinkV1Traffic();
            continue;
        }

        if (!isV1) {
            link->reportMavlinkV2Traffic();
            _updateCounters(mavlinkChannel, message);
        }
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

void MAVLinkProtocol::_updateCounters(uint8_t mavlinkChannel, const mavlink_message_t& message)
{
    _totalReceiveCounter[mavlinkChannel]++;

    const QPair<quint32, uint8_t> key(message.sysid, message.compid);
    auto& sequences = _lastIndex[mavlinkChannel];
    auto* const previous = sequences.object(key);
    uint8_t expectedSeq;
    if (!previous) {
        expectedSeq = message.seq;
    } else if (message.seq == *previous) {
        // v1/v2 of the same message share sequence numbers — duplicate seq isn't loss.
        return;
    } else {
        expectedSeq = *previous + 1;
    }

    uint64_t lostMessages;
    if (message.seq >= expectedSeq) {
        lostMessages = message.seq - expectedSeq;
    } else {
        lostMessages = static_cast<uint64_t>(message.seq) + 256ULL - expectedSeq;
    }
    _totalLossCounter[mavlinkChannel] += lostMessages;

    if (previous) {
        *previous = message.seq;
    } else {
        sequences.insert(key, new uint8_t(message.seq));
    }

    const uint64_t totalSent = _totalReceiveCounter[mavlinkChannel] + _totalLossCounter[mavlinkChannel];
    const float currentLossPercent = (static_cast<double>(_totalLossCounter[mavlinkChannel]) / totalSent) * 100.0f;
    _runningLossPercent[mavlinkChannel] = (currentLossPercent + _runningLossPercent[mavlinkChannel]) * 0.5f;
}

void MAVLinkProtocol::_forward(const mavlink_message_t& message)
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

    // Strip signature on forward: foreign key would BAD_SIGNATURE on downstream signing-aware parsers.
    const QByteArray bytes = MAVLinkSigning::serializeUnsignedCopy(message);
    (void)forwardingLink->writeBytesThreadSafe(bytes.constData(), bytes.size());
}

void MAVLinkProtocol::_forwardSupport(const mavlink_message_t& message)
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

    const QByteArray bytes = MAVLinkSigning::serializeUnsignedCopy(message);
    (void)forwardingSupportLink->writeBytesThreadSafe(bytes.constData(), bytes.size());
}

void MAVLinkProtocol::_logData(LinkInterface* link, const mavlink_message_t& message)
{
    // Foreign traffic is still logged, but must not discover vehicles or set the armed state.
    bool addressedElsewhere = false;
    if (message.incompat_flags & MAVLINK_IFLAG_TARGET32) {
        const uint8_t targetComponent = mavlink_msg_get_target_compid(&message, mavlink_get_msg_entry(message.msgid));
        addressedElsewhere = (message.target_sysid != 0 && message.target_sysid != getSystemId()) ||
                             (targetComponent != 0 && targetComponent != getComponentId());
    }

    // Checked before the log is known to be open so that the heartbeat that opens it counts. Replayed heartbeats
    // are skipped: a live vehicle connected while the replay is paused would otherwise inherit their armed state.
    if ((message.msgid == MAVLINK_MSG_ID_HEARTBEAT) && !_vehicleWasArmed && !addressedElsewhere &&
        !link->isLogReplay()) {
        if (mavlink_msg_heartbeat_get_base_mode(&message) & MAV_MODE_FLAG_DECODE_POSITION_SAFETY) {
            _vehicleWasArmed = true;
        }
    }

    if (!_logSuspendError && !_logSuspendReplay && _logFile->isOpen()) {
        // MAVLink spec §Logging: omit SETUP_SIGNING (contains secret key)
        if (message.msgid != MAVLINK_MSG_ID_SETUP_SIGNING) {
            // MAVLink spec §Logging: strip signature block from logged packets.
            const QByteArray msgBytes = MAVLinkSigning::serializeUnsignedCopy(message);
            const quint64 timestamp = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch() * 1000);
            QByteArray log_data;
            log_data.resize(static_cast<qsizetype>(sizeof(timestamp)) + msgBytes.size());
            qToBigEndian(timestamp, reinterpret_cast<uint8_t*>(log_data.data()));
            std::memcpy(log_data.data() + sizeof(timestamp), msgBytes.constData(), msgBytes.size());
            if (_logFile->write(log_data) != log_data.size()) {
                const QString logErrorMessage =
                    QStringLiteral("MAVLink Logging failed. Could not write to file %1, logging disabled.")
                        .arg(_logFile->fileName());
                QGC::showAppMessage(logErrorMessage, getName());
                _stopLogging();
                _logSuspendError = true;
            }
        }

        // A vehicle on a high latency link may never send a heartbeat.
        const bool vehiclePresenceMessage = (message.msgid == MAVLINK_MSG_ID_HEARTBEAT) ||
                                            (message.msgid == MAVLINK_MSG_ID_HIGH_LATENCY) ||
                                            (message.msgid == MAVLINK_MSG_ID_HIGH_LATENCY2);
        if (vehiclePresenceMessage || _logKeepCheckPending) {
            _promoteLogFileIfKept();
        }
    }

    if (addressedElsewhere) {
        return;
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

bool MAVLinkProtocol::_updateStatus(LinkInterface* link, const SharedLinkInterfacePtr linkPtr, uint8_t mavlinkChannel,
                                    const mavlink_message_t& message)
{
    if ((_totalReceiveCounter[mavlinkChannel] % 31) == 0) {
        const uint64_t totalSent = _totalReceiveCounter[mavlinkChannel] + _totalLossCounter[mavlinkChannel];
        emit mavlinkMessageStatus(message.sysid, totalSent, _totalReceiveCounter[mavlinkChannel],
                                  _totalLossCounter[mavlinkChannel], _runningLossPercent[mavlinkChannel]);
    }

    emit messageReceived(link, message);

    if (linkPtr.use_count() == 1) {
        return false;
    }

    return true;
}

bool MAVLinkProtocol::_closeLogFile()
{
    if (!_logFile->isOpen()) {
        return false;
    }

    if (_logFile->size() == 0) {
        (void) _logFile->remove();
        return false;
    }

    (void) _logFile->flush();
    _logFile->close();
    return true;
}

void MAVLinkProtocol::_startLogging()
{
    if (QGC::runningUnitTests() && !_loggingEnabledForTest) {
        return;
    }

    AppSettings* const appSettings = SettingsManager::instance()->appSettings();
    if (appSettings->disableAllPersistence()->rawValue().toBool()) {
        return;
    }

    if (_logFile->isOpen()) {
        return;
    }

    if (_logSuspendReplay || _logSuspendError) {
        return;
    }

    // Called again on every heartbeat until the log opens, so a failure is only reported once.
    const auto reportStartError = [this](const QString& message, const QString& title = QString()) {
        if (!_logStartErrorShown) {
            _logStartErrorShown = true;
            QGC::showAppMessage(message, title);
        }
    };

    const QString saveDirPath = appSettings->telemetrySavePath();
    const QString savePathError = _telemetrySavePathError(saveDirPath);
    if (!savePathError.isEmpty()) {
        reportStartError(tr("Telemetry log save path is not valid. %1").arg(savePathError));
        return;
    }

    const QString startTimeString = QDateTime::currentDateTime().toString(QLatin1String(_logStartTimeFormat));
    const QString inProgressFileName = QStringLiteral("%1.%2.%3")
                                           .arg(startTimeString, QLatin1String(AppSettings::telemetryFileExtension),
                                                QLatin1String(_inProgressLogSuffix));
    _logFile->setFileName(QDir(saveDirPath).filePath(inProgressFileName));
    if (!_logFile->open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        const QString message = QStringLiteral(
                                    "Opening Flight Data file for writing failed. "
                                    "Unable to write to %1. Please choose a different file location.")
                                    .arg(_logFile->fileName());
        reportStartError(message, getName());
        return;
    }

    qCDebug(MAVLinkProtocolLog) << "In-progress log" << _logFile->fileName();

    _logStartErrorShown = false;
    _logPromoteFailed = false;
    _logKeepCheckPending = true;
    _logFinalizePending = true;
}

void MAVLinkProtocol::_stopLogging()
{
    // A log that could not be reopened after its rename is already closed but still has to be finalized.
    if (_logFinalizePending) {
        _logFinalizePending = false;
        // _closeLogFile() removes an empty log
        const bool logHasData = !_logFile->isOpen() || _closeLogFile();
        if (logHasData) {
            if (!_keepLog()) {
                (void) _logFile->remove();
            } else if (!_promoteLogFile()) {
                QGC::showAppMessage(tr("Unable to save telemetry log '%1': '%2'. Rename it to .%3 to keep it, "
                                       "otherwise it is deleted the next time the application starts.")
                                        .arg(_logFile->fileName(), _logFile->errorString(),
                                             QLatin1String(AppSettings::telemetryFileExtension)));
            }
        }
    }

    _vehicleWasArmed = false;
    _logStartErrorShown = false;
    _logSuspendError = false;
}

bool MAVLinkProtocol::_logFileInProgress() const
{
    return _logFile->fileName().endsWith(QLatin1String(_inProgressLogSuffix));
}

bool MAVLinkProtocol::_keepLog() const
{
    MavlinkSettings* const mavlinkSettings = SettingsManager::instance()->mavlinkSettings();
    return (_vehicleWasArmed || mavlinkSettings->telemetrySaveNotArmed()->rawValue().toBool()) &&
           mavlinkSettings->telemetrySave()->rawValue().toBool() &&
           !SettingsManager::instance()->appSettings()->disableAllPersistence()->rawValue().toBool();
}

void MAVLinkProtocol::_promoteLogFileIfKept()
{
    // A crash would leave an empty .tlog behind, and a rename that failed is only retried when logging stops.
    if (!_logFile->isOpen() || !_logFileInProgress() || _logPromoteFailed || (_logFile->pos() == 0)) {
        return;
    }

    _logKeepCheckPending = false;
    if (_keepLog() && !_promoteLogFile()) {
        _logPromoteFailed = true;
    }
}

bool MAVLinkProtocol::_promoteLogFile()
{
    if (!_logFileInProgress()) {
        return true;
    }

    const QFileInfo inProgressInfo(_logFile->fileName());
    const QDir saveDir = inProgressInfo.dir();
    const QString startTimeString = inProgressInfo.baseName();

    const QString nameFormat("%1%2.%3");
    int tryIndex = 1;
    QString saveFileName = nameFormat.arg(startTimeString, QString(), AppSettings::telemetryFileExtension);
    while (saveDir.exists(saveFileName)) {
        saveFileName =
            nameFormat.arg(startTimeString, QStringLiteral(".%1").arg(tryIndex++), AppSettings::telemetryFileExtension);
    }

    // QFile::rename() closes the file first, which Windows needs in order to rename it.
    const bool wasOpen = _logFile->isOpen();
    const bool renamed = _logFile->rename(saveDir.filePath(saveFileName));
    if (renamed) {
        // Until it is saved the log keeps the mode the umask gave it, 0666 & ~umask.
        constexpr QFileDevice::Permissions perms =
            QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther;
        (void) _logFile->setPermissions(perms);
    } else {
        qCWarning(MAVLinkProtocolLog) << "Unable to rename log" << _logFile->fileName() << "to" << saveFileName
                                      << "error:" << _logFile->errorString();
    }

    if (wasOpen && !_logFile->isOpen() && !_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        QGC::showAppMessage(tr("MAVLink Logging failed. Could not reopen file '%1': '%2', logging disabled.")
                                .arg(_logFile->fileName(), _logFile->errorString()),
                            getName());
        _logSuspendError = true;
    }

    return renamed;
}

void MAVLinkProtocol::removeOrphanedLogFiles()
{
    const QString saveDirPath = SettingsManager::instance()->appSettings()->telemetrySavePath();
    if (saveDirPath.isEmpty()) {
        return;
    }

    const QString filter = QStringLiteral("*.%1.%2").arg(QLatin1String(AppSettings::telemetryFileExtension),
                                                         QLatin1String(_inProgressLogSuffix));
    const QFileInfoList fileInfoList = QDir(saveDirPath).entryInfoList(QStringList(filter), QDir::Files);
    for (const QFileInfo& fileInfo : fileInfoList) {
        qCDebug(MAVLinkProtocolLog) << "Removing orphaned log file" << fileInfo.filePath();
        (void) QFile::remove(fileInfo.filePath());
    }
}

QString MAVLinkProtocol::_telemetrySavePathError(const QString& saveDirPath)
{
    if (saveDirPath.isEmpty()) {
        return tr("Application save directory is not set or does not exist.");
    }

    if (!QDir(saveDirPath).exists()) {
        return tr("Telemetry save directory \"%1\" does not exist.").arg(saveDirPath);
    }

    return QString();
}

void MAVLinkProtocol::_vehicleCountChanged()
{
    if (MultiVehicleManager::instance()->vehicles()->count() == 0) {
        _stopLogging();
    }
}

quint32 MAVLinkProtocol::getSystemId() const
{
    return SettingsManager::instance()->mavlinkSettings()->gcsMavlinkSystemID()->rawValue().toUInt();
}
