/*===================================================================
======================================================================*/

/**
 * @file
 *   @brief Implementation of class MAVLinkProtocol
 *   @author Lorenz Meier <mail@qgroundcontrol.org>
 */

#include <inttypes.h>
#include <iostream>

#include <QDebug>
#include <QTime>
#include <QApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QtEndian>
#include <QMetaType>

#include "MAVLinkProtocol.h"
#include "UASInterface.h"
#include "UASInterface.h"
#include "UAS.h"
#include "LinkManager.h"
#include "QGCMAVLink.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "MultiVehicleManager.h"

Q_DECLARE_METATYPE(mavlink_message_t)

QGC_LOGGING_CATEGORY(MAVLinkProtocolLog, "MAVLinkProtocolLog")

#ifndef __mobile__
const char* MAVLinkProtocol::_tempLogFileTemplate = "FlightDataXXXXXX"; ///< Template for temporary log file
const char* MAVLinkProtocol::_logFileExtension = "mavlink";             ///< Extension for log files
#endif

/**
 * The default constructor will create a new MAVLink object sending heartbeats at
 * the MAVLINK_HEARTBEAT_DEFAULT_RATE to all connected links.
 */
MAVLinkProtocol::MAVLinkProtocol(QGCApplication* app)
    : QGCTool(app)
    , m_multiplexingEnabled(false)
    , m_authEnabled(false)
    , m_enable_version_check(true)
    , m_paramRetransmissionTimeout(350)
    , m_paramRewriteTimeout(500)
    , m_paramGuardEnabled(true)
    , m_actionGuardEnabled(false)
    , m_actionRetransmissionTimeout(100)
    , versionMismatchIgnore(false)
    , systemId(QGC::defaultSystemId)
#ifndef __mobile__
    , _logSuspendError(false)
    , _logSuspendReplay(false)
    , _logPromptForSave(false)
    , _tempLogFile(QString("%2.%3").arg(_tempLogFileTemplate).arg(_logFileExtension))
#endif
    , _linkMgr(NULL)
    , _multiVehicleManager(NULL)
{
    memset(&totalReceiveCounter, 0, sizeof(totalReceiveCounter));
    memset(&totalLossCounter, 0, sizeof(totalLossCounter));
    memset(&totalErrorCounter, 0, sizeof(totalErrorCounter));
    memset(&currReceiveCounter, 0, sizeof(currReceiveCounter));
    memset(&currLossCounter, 0, sizeof(currLossCounter));
}

MAVLinkProtocol::~MAVLinkProtocol()
{
    storeSettings();
    
#ifndef __mobile__
    _closeLogFile();
#endif
}

void MAVLinkProtocol::setToolbox(QGCToolbox *toolbox)
{
   QGCTool::setToolbox(toolbox);

   _linkMgr =               _toolbox->linkManager();
   _multiVehicleManager =   _toolbox->multiVehicleManager();

   qRegisterMetaType<mavlink_message_t>("mavlink_message_t");

   m_authKey = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
   loadSettings();

   // All the *Counter variables are not initialized here, as they should be initialized
   // on a per-link basis before those links are used. @see resetMetadataForLink().

   // Initialize the list for tracking dropped messages to invalid.
   for (int i = 0; i < 256; i++)
   {
       for (int j = 0; j < 256; j++)
       {
           lastIndex[i][j] = -1;
       }
   }

   connect(this, &MAVLinkProtocol::protocolStatusMessage, _app, &QGCApplication::criticalMessageBoxOnMainThread);
#ifndef __mobile__
   connect(this, &MAVLinkProtocol::saveTempFlightDataLog, _app, &QGCApplication::saveTempFlightDataLogOnMainThread);
#endif

   connect(_multiVehicleManager->vehicles(), &QmlObjectListModel::countChanged, this, &MAVLinkProtocol::_vehicleCountChanged);

   emit versionCheckChanged(m_enable_version_check);
}

void MAVLinkProtocol::loadSettings()
{
    // Load defaults from settings
    QSettings settings;
    settings.beginGroup("QGC_MAVLINK_PROTOCOL");
    enableVersionCheck(settings.value("VERSION_CHECK_ENABLED", m_enable_version_check).toBool());
    enableMultiplexing(settings.value("MULTIPLEXING_ENABLED", m_multiplexingEnabled).toBool());

    // Only set system id if it was valid
    int temp = settings.value("GCS_SYSTEM_ID", systemId).toInt();
    if (temp > 0 && temp < 256)
    {
        systemId = temp;
    }

    // Set auth key
    m_authKey = settings.value("GCS_AUTH_KEY", m_authKey).toString();
    enableAuth(settings.value("GCS_AUTH_ENABLED", m_authEnabled).toBool());

    // Parameter interface settings
    bool ok;
    temp = settings.value("PARAMETER_RETRANSMISSION_TIMEOUT", m_paramRetransmissionTimeout).toInt(&ok);
    if (ok) m_paramRetransmissionTimeout = temp;
    temp = settings.value("PARAMETER_REWRITE_TIMEOUT", m_paramRewriteTimeout).toInt(&ok);
    if (ok) m_paramRewriteTimeout = temp;
    m_paramGuardEnabled = settings.value("PARAMETER_TRANSMISSION_GUARD_ENABLED", m_paramGuardEnabled).toBool();
    settings.endGroup();
}

void MAVLinkProtocol::storeSettings()
{
    // Store settings
    QSettings settings;
    settings.beginGroup("QGC_MAVLINK_PROTOCOL");
    settings.setValue("VERSION_CHECK_ENABLED", m_enable_version_check);
    settings.setValue("MULTIPLEXING_ENABLED", m_multiplexingEnabled);
    settings.setValue("GCS_SYSTEM_ID", systemId);
    settings.setValue("GCS_AUTH_KEY", m_authKey);
    settings.setValue("GCS_AUTH_ENABLED", m_authEnabled);
    // Parameter interface settings
    settings.setValue("PARAMETER_RETRANSMISSION_TIMEOUT", m_paramRetransmissionTimeout);
    settings.setValue("PARAMETER_REWRITE_TIMEOUT", m_paramRewriteTimeout);
    settings.setValue("PARAMETER_TRANSMISSION_GUARD_ENABLED", m_paramGuardEnabled);
    settings.endGroup();
}

void MAVLinkProtocol::resetMetadataForLink(const LinkInterface *link)
{
    int channel = link->getMavlinkChannel();
    totalReceiveCounter[channel] = 0;
    totalLossCounter[channel] = 0;
    totalErrorCounter[channel] = 0;
    currReceiveCounter[channel] = 0;
    currLossCounter[channel] = 0;
}

/**
 * This method parses all incoming bytes and constructs a MAVLink packet.
 * It can handle multiple links in parallel, as each link has it's own buffer/
 * parsing state machine.
 * @param link The interface to read from
 * @see LinkInterface
 **/
void MAVLinkProtocol::receiveBytes(LinkInterface* link, QByteArray b)
{
    // Since receiveBytes signals cross threads we can end up with signals in the queue
    // that come through after the link is disconnected. For these we just drop the data
    // since the link is closed.
    if (!_linkMgr->links()->contains(link)) {
        return;
    }
    
//    receiveMutex.lock();
    mavlink_message_t message;
    mavlink_status_t status;

    int mavlinkChannel = link->getMavlinkChannel();

    static int mavlink09Count = 0;
    static int nonmavlinkCount = 0;
    static bool decodedFirstPacket = false;
    static bool warnedUser = false;
    static bool checkedUserNonMavlink = false;
    static bool warnedUserNonMavlink = false;

    for (int position = 0; position < b.size(); position++) {
        unsigned int decodeState = mavlink_parse_char(mavlinkChannel, (uint8_t)(b[position]), &message, &status);

        if ((uint8_t)b[position] == 0x55) mavlink09Count++;
        if ((mavlink09Count > 100) && !decodedFirstPacket && !warnedUser)
        {
            warnedUser = true;
            // Obviously the user tries to use a 0.9 autopilot
            // with QGroundControl built for version 1.0
            emit protocolStatusMessage(tr("MAVLink Protocol"), tr("There is a MAVLink Version or Baud Rate Mismatch. "
                                                                  "Your MAVLink device seems to use the deprecated version 0.9, while QGroundControl only supports version 1.0+. "
                                                                  "Please upgrade the MAVLink version of your autopilot. "
                                                                  "If your autopilot is using version 1.0, check if the baud rates of QGroundControl and your autopilot are the same."));
        }

        if (decodeState == 0 && !decodedFirstPacket)
        {
            nonmavlinkCount++;
            if (nonmavlinkCount > 2000 && !warnedUserNonMavlink)
            {
                //2000 bytes with no mavlink message. Are we connected to a mavlink capable device?
                if (!checkedUserNonMavlink)
                {
                    link->requestReset();
                    checkedUserNonMavlink = true;
                }
                else
                {
                    warnedUserNonMavlink = true;
                    emit protocolStatusMessage(tr("MAVLink Protocol"), tr("There is a MAVLink Version or Baud Rate Mismatch. "
                                                                          "Please check if the baud rates of QGroundControl and your autopilot are the same."));
                }
            }
        }
        if (decodeState == 1)
        {
            decodedFirstPacket = true;

            if(message.msgid == MAVLINK_MSG_ID_PING)
            {
                // process ping requests (tgt_system and tgt_comp must be zero)
                mavlink_ping_t ping;
                mavlink_msg_ping_decode(&message, &ping);
                if(!ping.target_system && !ping.target_component)
                {
                    mavlink_message_t msg;
                    mavlink_msg_ping_pack(getSystemId(), getComponentId(), &msg, ping.time_usec, ping.seq, message.sysid, message.compid);
                    _sendMessage(msg);
                }
            }

            if(message.msgid == MAVLINK_MSG_ID_RADIO_STATUS)
            {
                // process telemetry status message
                mavlink_radio_status_t rstatus;
                mavlink_msg_radio_status_decode(&message, &rstatus);
                int rssi = rstatus.rssi,
                    remrssi = rstatus.remrssi;
                // 3DR Si1k radio needs rssi fields to be converted to dBm
                if (message.sysid == '3' && message.compid == 'D') {
                    /* Per the Si1K datasheet figure 23.25 and SI AN474 code
                     * samples the relationship between the RSSI register
                     * and received power is as follows:
                     *
                     *                       10
                     * inputPower = rssi * ------ 127
                     *                       19
                     *
                     * Additionally limit to the only realistic range [-120,0] dBm
                     */
                    rssi    = qMin(qMax(qRound(static_cast<qreal>(rssi)    / 1.9 - 127.0), - 120), 0);
                    remrssi = qMin(qMax(qRound(static_cast<qreal>(remrssi) / 1.9 - 127.0), - 120), 0);
                } else {
                    rssi = (int8_t) rstatus.rssi;
                    remrssi = (int8_t) rstatus.remrssi;
                }

                emit radioStatusChanged(link, rstatus.rxerrors, rstatus.fixed, rssi, remrssi,
                    rstatus.txbuf, rstatus.noise, rstatus.remnoise);
            }

#ifndef __mobile__
            // Log data
            
            if (!_logSuspendError && !_logSuspendReplay && _tempLogFile.isOpen()) {
                uint8_t buf[MAVLINK_MAX_PACKET_LEN+sizeof(quint64)];

                // Write the uint64 time in microseconds in big endian format before the message.
                // This timestamp is saved in UTC time. We are only saving in ms precision because
                // getting more than this isn't possible with Qt without a ton of extra code.
                quint64 time = (quint64)QDateTime::currentMSecsSinceEpoch() * 1000;
                qToBigEndian(time, buf);

                // Then write the message to the buffer
                int len = mavlink_msg_to_send_buffer(buf + sizeof(quint64), &message);

                // Determine how many bytes were written by adding the timestamp size to the message size
                len += sizeof(quint64);

                // Now write this timestamp/message pair to the log.
                QByteArray b((const char*)buf, len);
                if(_tempLogFile.write(b) != len)
                {
                    // If there's an error logging data, raise an alert and stop logging.
                    emit protocolStatusMessage(tr("MAVLink Protocol"), tr("MAVLink Logging failed. Could not write to file %1, logging disabled.").arg(_tempLogFile.fileName()));
                    _stopLogging();
                    _logSuspendError = true;
                }
                
                // Check for the vehicle arming going by. This is used to trigger log save.
                if (!_logPromptForSave && message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                    mavlink_heartbeat_t state;
                    mavlink_msg_heartbeat_decode(&message, &state);
                    if (state.base_mode & MAV_MODE_FLAG_DECODE_POSITION_SAFETY) {
                        _logPromptForSave = true;
                    }
                }
            }
#endif

            if (message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
#ifndef __mobile__
                // Start loggin on first heartbeat
                _startLogging();
#endif

                mavlink_heartbeat_t heartbeat;
                mavlink_msg_heartbeat_decode(&message, &heartbeat);
                emit vehicleHeartbeatInfo(link, message.sysid, heartbeat.mavlink_version, heartbeat.autopilot, heartbeat.type);
            }

            // Increase receive counter
            totalReceiveCounter[mavlinkChannel]++;
            currReceiveCounter[mavlinkChannel]++;

            // Determine what the next expected sequence number is, accounting for
            // never having seen a message for this system/component pair.
            int lastSeq = lastIndex[message.sysid][message.compid];
            int expectedSeq = (lastSeq == -1) ? message.seq : (lastSeq + 1);

            // And if we didn't encounter that sequence number, record the error
            if (message.seq != expectedSeq)
            {

                // Determine how many messages were skipped
                int lostMessages = message.seq - expectedSeq;

                // Out of order messages or wraparound can cause this, but we just ignore these conditions for simplicity
                if (lostMessages < 0)
                {
                    lostMessages = 0;
                }

                // And log how many were lost for all time and just this timestep
                totalLossCounter[mavlinkChannel] += lostMessages;
                currLossCounter[mavlinkChannel] += lostMessages;
            }

            // And update the last sequence number for this system/component pair
            lastIndex[message.sysid][message.compid] = expectedSeq;

            // Update on every 32th packet
            if ((totalReceiveCounter[mavlinkChannel] & 0x1F) == 0)
            {
                // Calculate new loss ratio
                // Receive loss
                float receiveLossPercent = (double)currLossCounter[mavlinkChannel]/(double)(currReceiveCounter[mavlinkChannel]+currLossCounter[mavlinkChannel]);
                receiveLossPercent *= 100.0f;
                currLossCounter[mavlinkChannel] = 0;
                currReceiveCounter[mavlinkChannel] = 0;
                emit receiveLossPercentChanged(message.sysid, receiveLossPercent);
                emit receiveLossTotalChanged(message.sysid, totalLossCounter[mavlinkChannel]);
            }

            // The packet is emitted as a whole, as it is only 255 - 261 bytes short
            // kind of inefficient, but no issue for a groundstation pc.
            // It buys as reentrancy for the whole code over all threads
            emit messageReceived(link, message);

            // Multiplex message if enabled
            if (m_multiplexingEnabled)
            {
                // Emit message on all links that are currently connected
                for (int i=0; i<_linkMgr->links()->count(); i++) {
                    LinkInterface* currLink = _linkMgr->links()->value<LinkInterface*>(i);

                    // Only forward this message to the other links,
                    // not the link the message was received on
                    if (currLink && currLink != link) _sendMessage(currLink, message, message.sysid, message.compid);
                }
            }
        }
    }
}

/**
 * @return The name of this protocol
 **/
QString MAVLinkProtocol::getName()
{
    return QString(tr("MAVLink protocol"));
}

/** @return System id of this application */
int MAVLinkProtocol::getSystemId()
{
    return systemId;
}

void MAVLinkProtocol::setSystemId(int id)
{
    systemId = id;
}

/** @return Component id of this application */
int MAVLinkProtocol::getComponentId()
{
    return QGC::defaultComponentId;
}

/**
 * @param message message to send
 */
void MAVLinkProtocol::_sendMessage(mavlink_message_t message)
{
    for (int i=0; i<_linkMgr->links()->count(); i++) {
        LinkInterface* link = _linkMgr->links()->value<LinkInterface*>(i);
        _sendMessage(link, message);
    }
}

/**
 * @param link the link to send the message over
 * @param message message to send
 */
void MAVLinkProtocol::_sendMessage(LinkInterface* link, mavlink_message_t message)
{
    // Create buffer
    static uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    // Rewriting header to ensure correct link ID is set
    static uint8_t messageKeys[256] = MAVLINK_MESSAGE_CRCS;
    mavlink_finalize_message_chan(&message, this->getSystemId(), this->getComponentId(), link->getMavlinkChannel(), message.len, messageKeys[message.msgid]);
    // Write message into buffer, prepending start sign
    int len = mavlink_msg_to_send_buffer(buffer, &message);
    // If link is connected
    if (link->isConnected())
    {
        // Send the portion of the buffer now occupied by the message
        link->writeBytesSafe((const char*)buffer, len);
    }
}

/**
 * @param link the link to send the message over
 * @param message message to send
 * @param systemid id of the system the message is originating from
 * @param componentid id of the component the message is originating from
 */
void MAVLinkProtocol::_sendMessage(LinkInterface* link, mavlink_message_t message, quint8 systemid, quint8 componentid)
{
    // Create buffer
    static uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    // Rewriting header to ensure correct link ID is set
    static uint8_t messageKeys[256] = MAVLINK_MESSAGE_CRCS;
    mavlink_finalize_message_chan(&message, systemid, componentid, link->getMavlinkChannel(), message.len, messageKeys[message.msgid]);
    // Write message into buffer, prepending start sign
    int len = mavlink_msg_to_send_buffer(buffer, &message);
    // If link is connected
    if (link->isConnected())
    {
        // Send the portion of the buffer now occupied by the message
        link->writeBytesSafe((const char*)buffer, len);
    }
}

void MAVLinkProtocol::enableMultiplexing(bool enabled)
{
    bool changed = false;
    if (enabled != m_multiplexingEnabled) changed = true;

    m_multiplexingEnabled = enabled;
    if (changed) emit multiplexingChanged(m_multiplexingEnabled);
}

void MAVLinkProtocol::enableAuth(bool enable)
{
    bool changed = false;
    m_authEnabled = enable;
    if (m_authEnabled != enable) {
        changed = true;
    }
    if (changed) emit authChanged(m_authEnabled);
}

void MAVLinkProtocol::enableParamGuard(bool enabled)
{
    if (enabled != m_paramGuardEnabled) {
        m_paramGuardEnabled = enabled;
        emit paramGuardChanged(m_paramGuardEnabled);
    }
}

void MAVLinkProtocol::enableActionGuard(bool enabled)
{
    if (enabled != m_actionGuardEnabled) {
        m_actionGuardEnabled = enabled;
        emit actionGuardChanged(m_actionGuardEnabled);
    }
}

void MAVLinkProtocol::setParamRetransmissionTimeout(int ms)
{
    if (ms != m_paramRetransmissionTimeout) {
        m_paramRetransmissionTimeout = ms;
        emit paramRetransmissionTimeoutChanged(m_paramRetransmissionTimeout);
    }
}

void MAVLinkProtocol::setParamRewriteTimeout(int ms)
{
    if (ms != m_paramRewriteTimeout) {
        m_paramRewriteTimeout = ms;
        emit paramRewriteTimeoutChanged(m_paramRewriteTimeout);
    }
}

void MAVLinkProtocol::setActionRetransmissionTimeout(int ms)
{
    if (ms != m_actionRetransmissionTimeout) {
        m_actionRetransmissionTimeout = ms;
        emit actionRetransmissionTimeoutChanged(m_actionRetransmissionTimeout);
    }
}

void MAVLinkProtocol::enableVersionCheck(bool enabled)
{
    m_enable_version_check = enabled;
    emit versionCheckChanged(enabled);
}

void MAVLinkProtocol::_vehicleCountChanged(int count)
{
#ifndef __mobile__
    if (count == 0) {
        // Last vehicle is gone, close out logging
        _stopLogging();
    }
#else
    Q_UNUSED(count);
#endif
}

#ifndef __mobile__
/// @brief Closes the log file if it is open
bool MAVLinkProtocol::_closeLogFile(void)
{
    if (_tempLogFile.isOpen()) {
        if (_tempLogFile.size() == 0) {
            // Don't save zero byte files
            _tempLogFile.remove();
            return false;
        } else {
            _tempLogFile.flush();
            _tempLogFile.close();
            return true;
        }
    }
    
    return false;
}

void MAVLinkProtocol::_startLogging(void)
{
    if (!_tempLogFile.isOpen()) {
        if (!_logSuspendReplay) {
            if (!_tempLogFile.open()) {
                emit protocolStatusMessage(tr("MAVLink Protocol"), tr("Opening Flight Data file for writing failed. "
                                                                      "Unable to write to %1. Please choose a different file location.").arg(_tempLogFile.fileName()));
                _closeLogFile();
                _logSuspendError = true;
                return;
            }

            if (_app->promptFlightDataSaveNotArmed()) {
                _logPromptForSave = true;
            }

            qDebug() << "Temp log" << _tempLogFile.fileName() << _logPromptForSave;

            _logSuspendError = false;
        }
    }
}

void MAVLinkProtocol::_stopLogging(void)
{
    if (_closeLogFile()) {
        // If the signals are not connected it means we are running a unit test. In that case just delete log files
        if (_logPromptForSave && _app->promptFlightDataSave()) {
            emit saveTempFlightDataLog(_tempLogFile.fileName());
        } else {
            QFile::remove(_tempLogFile.fileName());
        }
    }
    _logPromptForSave = false;
}

/// @brief Checks the temp directory for log files which may have been left there.
///         This could happen if QGC crashes without the temp log file being saved.
///         Give the user an option to save these orphaned files.
void MAVLinkProtocol::checkForLostLogFiles(void)
{
    QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    
    QString filter(QString("*.%1").arg(_logFileExtension));
    QFileInfoList fileInfoList = tempDir.entryInfoList(QStringList(filter), QDir::Files);
    qDebug() << "Orphaned log file count" << fileInfoList.count();
    
    foreach(const QFileInfo fileInfo, fileInfoList) {
        qDebug() << "Orphaned log file" << fileInfo.filePath();
        if (fileInfo.size() == 0) {
            // Delete all zero length files
            QFile::remove(fileInfo.filePath());
            continue;
        }

        // Give the user a chance to save the orphaned log file
        emit protocolStatusMessage(tr("Found unsaved Flight Data"),
                                   tr("This can happen if QGroundControl crashes during Flight Data collection. "
                                      "If you want to save the unsaved Flight Data, select the file you want to save it to. "
                                      "If you do not want to keep the Flight Data, select 'Cancel' on the next dialog."));
        emit saveTempFlightDataLog(fileInfo.filePath());
    }
}

void MAVLinkProtocol::suspendLogForReplay(bool suspend)
{
    _logSuspendReplay = suspend;
}

void MAVLinkProtocol::deleteTempLogFiles(void)
{
    QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    
    QString filter(QString("*.%1").arg(_logFileExtension));
    QFileInfoList fileInfoList = tempDir.entryInfoList(QStringList(filter), QDir::Files);
    
    foreach(const QFileInfo fileInfo, fileInfoList) {
        QFile::remove(fileInfo.filePath());
    }
}
#endif
