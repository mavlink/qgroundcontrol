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
#include "UASManager.h"
#include "UASInterface.h"
#include "UAS.h"
#include "configuration.h"
#include "LinkManager.h"
#include "QGCMAVLink.h"
#include "QGCMAVLinkUASFactory.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_METATYPE(mavlink_message_t)
IMPLEMENT_QGC_SINGLETON(MAVLinkProtocol, MAVLinkProtocol)
QGC_LOGGING_CATEGORY(MAVLinkProtocolLog, "MAVLinkProtocolLog")

const char* MAVLinkProtocol::_tempLogFileTemplate = "FlightDataXXXXXX"; ///< Template for temporary log file
const char* MAVLinkProtocol::_logFileExtension = "mavlink";             ///< Extension for log files

/**
 * The default constructor will create a new MAVLink object sending heartbeats at
 * the MAVLINK_HEARTBEAT_DEFAULT_RATE to all connected links.
 */
MAVLinkProtocol::MAVLinkProtocol(QObject* parent) :
    QGCSingleton(parent),
    m_multiplexingEnabled(false),
    m_authEnabled(false),
    m_enable_version_check(true),
    m_paramRetransmissionTimeout(350),
    m_paramRewriteTimeout(500),
    m_paramGuardEnabled(true),
    m_actionGuardEnabled(false),
    m_actionRetransmissionTimeout(100),
    versionMismatchIgnore(false),
    systemId(QGC::defaultSystemId),
    _logSuspendError(false),
    _logSuspendReplay(false),
    _logWasArmed(false),
    _tempLogFile(QString("%2.%3").arg(_tempLogFileTemplate).arg(_logFileExtension)),
    _linkMgr(LinkManager::instance()),
    _heartbeatRate(MAVLINK_HEARTBEAT_DEFAULT_RATE),
    _heartbeatsEnabled(true)
{
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
    
    // Start heartbeat timer, emitting a heartbeat at the configured rate
    connect(&_heartbeatTimer, &QTimer::timeout, this, &MAVLinkProtocol::sendHeartbeat);
    _heartbeatTimer.start(1000/_heartbeatRate);

    connect(this, &MAVLinkProtocol::protocolStatusMessage, qgcApp(), &QGCApplication::criticalMessageBoxOnMainThread);
    connect(this, &MAVLinkProtocol::saveTempFlightDataLog, qgcApp(), &QGCApplication::saveTempFlightDataLogOnMainThread);

    emit versionCheckChanged(m_enable_version_check);
}

MAVLinkProtocol::~MAVLinkProtocol()
{
    storeSettings();
    
    _closeLogFile();
}

void MAVLinkProtocol::loadSettings()
{
    // Load defaults from settings
    QSettings settings;
    settings.beginGroup("QGC_MAVLINK_PROTOCOL");
    enableHeartbeats(settings.value("HEARTBEATS_ENABLED", _heartbeatsEnabled).toBool());
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
    settings.setValue("HEARTBEATS_ENABLED", _heartbeatsEnabled);
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

void MAVLinkProtocol::linkConnected(void)
{
    LinkInterface* link = qobject_cast<LinkInterface*>(QObject::sender());
    Q_ASSERT(link);
    
    _linkStatusChanged(link, true);
}

void MAVLinkProtocol::linkDisconnected(void)
{
    LinkInterface* link = qobject_cast<LinkInterface*>(QObject::sender());
    Q_ASSERT(link);
    
    _linkStatusChanged(link, false);
}

void MAVLinkProtocol::_linkStatusChanged(LinkInterface* link, bool connected)
{
    qCDebug(MAVLinkProtocolLog) << "_linkStatusChanged" << QString("%1").arg((long)link, 0, 16) << connected;
    Q_ASSERT(link);
    
    if (connected) {
        foreach (SharedLinkInterface sharedLink, _connectedLinks) {
            Q_ASSERT(sharedLink.data() != link);
        }
        
        // Use the same shared pointer as LinkManager
        _connectedLinks.append(LinkManager::instance()->sharedPointerForLink(link));
        
        if (_connectedLinks.count() == 1) {
            // This is the first link, we need to start logging
            _startLogging();
        }
        
        // Send command to start MAVLink
        // XXX hacky but safe
        // Start NSH
        const char init[] = {0x0d, 0x0d, 0x0d};
        link->writeBytes(init, sizeof(init));
        const char* cmd = "sh /etc/init.d/rc.usb\n";
        link->writeBytes(cmd, strlen(cmd));
        link->writeBytes(init, 4);
    } else {
        bool found = false;
        for (int i=0; i<_connectedLinks.count(); i++) {
            if (_connectedLinks[i].data() == link) {
                found = true;
                _connectedLinks.removeAt(i);
                break;
            }
        }
        Q_UNUSED(found);
        Q_ASSERT(found);
        
        if (_connectedLinks.count() == 0) {
            // Last link is gone, close out logging
            _stopLogging();
        }
    }
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
    if (!LinkManager::instance()->containsLink(link)) {
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
                    sendMessage(msg);
                }
            }

            if(message.msgid == MAVLINK_MSG_ID_RADIO_STATUS)
            {
                // process telemetry status message
                mavlink_radio_status_t rstatus;
                mavlink_msg_radio_status_decode(&message, &rstatus);

                emit radioStatusChanged(link, rstatus.rxerrors, rstatus.fixed, rstatus.rssi, rstatus.remrssi,
                    rstatus.txbuf, rstatus.noise, rstatus.remnoise);
            }

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
                if (!_logWasArmed && message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
                    mavlink_heartbeat_t state;
                    mavlink_msg_heartbeat_decode(&message, &state);
                    if (state.base_mode & MAV_MODE_FLAG_DECODE_POSITION_SAFETY) {
                        _logWasArmed = true;
                    }
                }
            }

            // ORDER MATTERS HERE!
            // If the matching UAS object does not yet exist, it has to be created
            // before emitting the packetReceived signal

            UASInterface* uas = UASManager::instance()->getUASForId(message.sysid);

            // Check and (if necessary) create UAS object
            if (uas == NULL && message.msgid == MAVLINK_MSG_ID_HEARTBEAT)
            {
                // ORDER MATTERS HERE!
                // The UAS object has first to be created and connected,
                // only then the rest of the application can be made aware
                // of its existence, as it only then can send and receive
                // it's first messages.

                // Check if the UAS has the same id like this system
                if (message.sysid == getSystemId())
                {
                    emit protocolStatusMessage(tr("MAVLink Protocol"), tr("Warning: A second system is using the same system id (%1)").arg(getSystemId()));
                }

                // Create a new UAS based on the heartbeat received
                // Todo dynamically load plugin at run-time for MAV
                // WIKISEARCH:AUTOPILOT_TYPE_INSTANTIATION

                // First create new UAS object
                // Decode heartbeat message
                mavlink_heartbeat_t heartbeat;
                // Reset version field to 0
                heartbeat.mavlink_version = 0;
                mavlink_msg_heartbeat_decode(&message, &heartbeat);

                // Check if the UAS has a different protocol version
                if (m_enable_version_check && (heartbeat.mavlink_version != MAVLINK_VERSION))
                {
                    // Bring up dialog to inform user
                    if (!versionMismatchIgnore)
                    {
                        emit protocolStatusMessage(tr("MAVLink Protocol"), tr("The MAVLink protocol version on the MAV and QGroundControl mismatch! "
                                                                              "It is unsafe to use different MAVLink versions. "
                                                                              "QGroundControl therefore refuses to connect to system %1, which sends MAVLink version %2 (QGroundControl uses version %3).").arg(message.sysid).arg(heartbeat.mavlink_version).arg(MAVLINK_VERSION));
                        versionMismatchIgnore = true;
                    }

                    // Ignore this message and continue gracefully
                    continue;
                }

                // Create a new UAS object
                uas = QGCMAVLinkUASFactory::createUAS(this, link, message.sysid, (MAV_AUTOPILOT)heartbeat.autopilot);

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
                float receiveLoss = (double)currLossCounter[mavlinkChannel]/(double)(currReceiveCounter[mavlinkChannel]+currLossCounter[mavlinkChannel]);
                receiveLoss *= 100.0f;
                currLossCounter[mavlinkChannel] = 0;
                currReceiveCounter[mavlinkChannel] = 0;
                emit receiveLossChanged(message.sysid, receiveLoss);
            }

            // The packet is emitted as a whole, as it is only 255 - 261 bytes short
            // kind of inefficient, but no issue for a groundstation pc.
            // It buys as reentrancy for the whole code over all threads
            emit messageReceived(link, message);

            // Multiplex message if enabled
            if (m_multiplexingEnabled)
            {
                // Get all links connected to this unit
                QList<LinkInterface*> links = _linkMgr->getLinks();

                // Emit message on all links that are currently connected
                foreach (LinkInterface* currLink, links)
                {
                    // Only forward this message to the other links,
                    // not the link the message was received on
                    if (currLink != link) sendMessage(currLink, message, message.sysid, message.compid);
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
void MAVLinkProtocol::sendMessage(mavlink_message_t message)
{
    // Get all links connected to this unit
    QList<LinkInterface*> links = _linkMgr->getLinks();

    // Emit message on all links that are currently connected
    QList<LinkInterface*>::iterator i;
    for (i = links.begin(); i != links.end(); ++i)
    {
        sendMessage(*i, message);
//        qDebug() << __FILE__ << __LINE__ << "SENT MESSAGE OVER" << ((LinkInterface*)*i)->getName() << "LIST SIZE:" << links.size();
    }
}

/**
 * @param link the link to send the message over
 * @param message message to send
 */
void MAVLinkProtocol::sendMessage(LinkInterface* link, mavlink_message_t message)
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
        link->writeBytes((const char*)buffer, len);
    }
}

/**
 * @param link the link to send the message over
 * @param message message to send
 * @param systemid id of the system the message is originating from
 * @param componentid id of the component the message is originating from
 */
void MAVLinkProtocol::sendMessage(LinkInterface* link, mavlink_message_t message, quint8 systemid, quint8 componentid)
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
        link->writeBytes((const char*)buffer, len);
    }
}

/**
 * The heartbeat is sent out of order and does not reset the
 * periodic heartbeat emission. It will be just sent in addition.
 * @return mavlink_message_t heartbeat message sent on serial link
 */
void MAVLinkProtocol::sendHeartbeat()
{
    if (_heartbeatsEnabled)
    {
        mavlink_message_t beat;
        mavlink_msg_heartbeat_pack(getSystemId(), getComponentId(),&beat, MAV_TYPE_GCS, MAV_AUTOPILOT_INVALID, MAV_MODE_MANUAL_ARMED, 0, MAV_STATE_ACTIVE);
        sendMessage(beat);
    }
    if (m_authEnabled)
    {
        mavlink_message_t msg;
        mavlink_auth_key_t auth;
        memset(&auth, 0, sizeof(auth));
        memcpy(auth.key, m_authKey.toStdString().c_str(), qMin(m_authKey.length(), MAVLINK_MSG_AUTH_KEY_FIELD_KEY_LEN));
        mavlink_msg_auth_key_encode(getSystemId(), getComponentId(), &msg, &auth);
        sendMessage(msg);
    }
}

/** @param enabled true to enable heartbeats emission at _heartbeatRate, false to disable */
void MAVLinkProtocol::enableHeartbeats(bool enabled)
{
    _heartbeatsEnabled = enabled;
    emit heartbeatChanged(enabled);
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

/**
 * The default rate is 1 Hertz.
 *
 * @param rate heartbeat rate in hertz (times per second)
 */
void MAVLinkProtocol::setHeartbeatRate(int rate)
{
    _heartbeatRate = rate;
    _heartbeatTimer.setInterval(1000/_heartbeatRate);
}

/** @return heartbeat rate in Hertz */
int MAVLinkProtocol::getHeartbeatRate()
{
    return _heartbeatRate;
}

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
    Q_ASSERT(!_tempLogFile.isOpen());

    if (!_logSuspendReplay) {
        if (!_tempLogFile.open()) {
            emit protocolStatusMessage(tr("MAVLink Protocol"), tr("Opening Flight Data file for writing failed. "
                                                                  "Unable to write to %1. Please choose a different file location.").arg(_tempLogFile.fileName()));
            _closeLogFile();
            _logSuspendError = true;
            return;
        }
    
        qDebug() << "Temp log" << _tempLogFile.fileName();
        
        _logSuspendError = false;
    }
}

void MAVLinkProtocol::_stopLogging(void)
{
    if (_closeLogFile()) {
        // If the signals are not connected it means we are running a unit test. In that case just delete log files
        if (_logWasArmed && qgcApp()->promptFlightDataSave()) {
            emit saveTempFlightDataLog(_tempLogFile.fileName());
        } else {
            QFile::remove(_tempLogFile.fileName());
        }
    }
    _logWasArmed = false;
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
