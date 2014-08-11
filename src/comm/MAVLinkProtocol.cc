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
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QtEndian>
#include <QMetaType>

#include "MAVLinkProtocol.h"
#include "UASInterface.h"
#include "UASManager.h"
#include "UASInterface.h"
#include "UAS.h"
#include "PxQuadMAV.h"
#include "ArduPilotMegaMAV.h"
#include "configuration.h"
#include "LinkManager.h"
#include "QGCMAVLink.h"
#include "QGCMAVLinkUASFactory.h"
#include "QGC.h"

Q_DECLARE_METATYPE(mavlink_message_t)

/**
 * The default constructor will create a new MAVLink object sending heartbeats at
 * the MAVLINK_HEARTBEAT_DEFAULT_RATE to all connected links.
 */
MAVLinkProtocol::MAVLinkProtocol() :
    heartbeatTimer(NULL),
    heartbeatRate(MAVLINK_HEARTBEAT_DEFAULT_RATE),
    m_heartbeatsEnabled(true),
    m_multiplexingEnabled(false),
    m_authEnabled(false),
    m_loggingEnabled(false),
    m_logfile(NULL),
    m_enable_version_check(true),
    m_paramRetransmissionTimeout(350),
    m_paramRewriteTimeout(500),
    m_paramGuardEnabled(true),
    m_actionGuardEnabled(false),
    m_actionRetransmissionTimeout(100),
    versionMismatchIgnore(false),
    systemId(QGC::defaultSystemId),
    _should_exit(false)
{
    qRegisterMetaType<mavlink_message_t>("mavlink_message_t");

    m_authKey = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    loadSettings();
    moveToThread(this);

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

    start(QThread::HighPriority);

    emit versionCheckChanged(m_enable_version_check);
}

void MAVLinkProtocol::loadSettings()
{
    // Load defaults from settings
    QSettings settings;
    settings.sync();
    settings.beginGroup("QGC_MAVLINK_PROTOCOL");
    enableHeartbeats(settings.value("HEARTBEATS_ENABLED", m_heartbeatsEnabled).toBool());
    enableVersionCheck(settings.value("VERSION_CHECK_ENABLED", m_enable_version_check).toBool());
    enableMultiplexing(settings.value("MULTIPLEXING_ENABLED", m_multiplexingEnabled).toBool());

    // Only set logfile if there is a name present in settings
    if (settings.contains("LOGFILE_NAME") && m_logfile == NULL)
    {
        m_logfile = new QFile(settings.value("LOGFILE_NAME").toString());
    }
    else if (m_logfile == NULL)
    {
        m_logfile = new QFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/qgroundcontrol_packetlog.mavlink");
    }
    // Enable logging
    enableLogging(settings.value("LOGGING_ENABLED", m_loggingEnabled).toBool());

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
    settings.setValue("HEARTBEATS_ENABLED", m_heartbeatsEnabled);
    settings.setValue("LOGGING_ENABLED", m_loggingEnabled);
    settings.setValue("VERSION_CHECK_ENABLED", m_enable_version_check);
    settings.setValue("MULTIPLEXING_ENABLED", m_multiplexingEnabled);
    settings.setValue("GCS_SYSTEM_ID", systemId);
    settings.setValue("GCS_AUTH_KEY", m_authKey);
    settings.setValue("GCS_AUTH_ENABLED", m_authEnabled);
    if (m_logfile)
    {
        // Logfile exists, store the name
        settings.setValue("LOGFILE_NAME", m_logfile->fileName());
    }
    // Parameter interface settings
    settings.setValue("PARAMETER_RETRANSMISSION_TIMEOUT", m_paramRetransmissionTimeout);
    settings.setValue("PARAMETER_REWRITE_TIMEOUT", m_paramRewriteTimeout);
    settings.setValue("PARAMETER_TRANSMISSION_GUARD_ENABLED", m_paramGuardEnabled);
    settings.endGroup();
    settings.sync();
    //qDebug() << "Storing settings!";
}

MAVLinkProtocol::~MAVLinkProtocol()
{
    storeSettings();
    if (m_logfile)
    {
        if (m_logfile->isOpen())
        {
            m_logfile->flush();
            m_logfile->close();
        }
        delete m_logfile;
        m_logfile = NULL;
    }

    // Tell the thread to exit
    _should_exit = true;
    // Wait for it to exit
    wait();
}

/**
 * @brief Runs the thread
 *
 **/
void MAVLinkProtocol::run()
{
    heartbeatTimer = new QTimer();
    heartbeatTimer->moveToThread(this);
    // Start heartbeat timer, emitting a heartbeat at the configured rate
    connect(heartbeatTimer, SIGNAL(timeout()), this, SLOT(sendHeartbeat()));
    heartbeatTimer->start(1000/heartbeatRate);

    while(!_should_exit) {

        if (isFinished()) {
            delete heartbeatTimer;
            qDebug() << "MAVLINK WORKER DONE!";
            return;
        }

        QCoreApplication::processEvents();
        QGC::SLEEP::msleep(2);
    }
}

QString MAVLinkProtocol::getLogfileName()
{
    if (m_logfile)
    {
        return m_logfile->fileName();
    }
    else
    {
        return QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/qgroundcontrol_packetlog.mavlink";
    }
}

void MAVLinkProtocol::resetMetadataForLink(const LinkInterface *link)
{
    int linkId = link->getId();
    totalReceiveCounter[linkId] = 0;
    totalLossCounter[linkId] = 0;
    totalErrorCounter[linkId] = 0;
    currReceiveCounter[linkId] = 0;
    currLossCounter[linkId] = 0;
}

void MAVLinkProtocol::linkStatusChanged(bool connected)
{
    LinkInterface* link = qobject_cast<LinkInterface*>(QObject::sender());

    if (link) {
        if (connected) {
            // Send command to start MAVLink
            // XXX hacky but safe
            // Start NSH
            const char init[] = {0x0d, 0x0d, 0x0d};
            link->writeBytes(init, sizeof(init));
            const char* cmd = "sh /etc/init.d/rc.usb\n";
            link->writeBytes(cmd, strlen(cmd));
            link->writeBytes(init, 4);
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
//    receiveMutex.lock();
    mavlink_message_t message;
    mavlink_status_t status;

    // Cache the link ID for common use.
    int linkId = link->getId();

    static int mavlink09Count = 0;
    static int nonmavlinkCount = 0;
    static bool decodedFirstPacket = false;
    static bool warnedUser = false;
    static bool checkedUserNonMavlink = false;
    static bool warnedUserNonMavlink = false;

    // FIXME: Add check for if link->getId() >= MAVLINK_COMM_NUM_BUFFERS
    for (int position = 0; position < b.size(); position++) {
        unsigned int decodeState = mavlink_parse_char(linkId, (uint8_t)(b[position]), &message, &status);

        if ((uint8_t)b[position] == 0x55) mavlink09Count++;
        if ((mavlink09Count > 100) && !decodedFirstPacket && !warnedUser)
        {
            warnedUser = true;
            // Obviously the user tries to use a 0.9 autopilot
            // with QGroundControl built for version 1.0
            emit protocolStatusMessage("MAVLink Version or Baud Rate Mismatch", "Your MAVLink device seems to use the deprecated version 0.9, while QGroundControl only supports version 1.0+. Please upgrade the MAVLink version of your autopilot. If your autopilot is using version 1.0, check if the baud rates of QGroundControl and your autopilot are the same.");
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
                    emit protocolStatusMessage("MAVLink Baud Rate Mismatch", "Please check if the baud rates of QGroundControl and your autopilot are the same.");
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
            if (m_loggingEnabled && m_logfile)
            {
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
                if(m_logfile->write(b) != len)
                {
                    // If there's an error logging data, raise an alert and stop logging.
                    emit protocolStatusMessage(tr("MAVLink Logging failed"), tr("Could not write to file %1, disabling logging.").arg(m_logfile->fileName()));
                    enableLogging(false);
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
                    emit protocolStatusMessage(tr("SYSTEM ID CONFLICT!"), tr("Warning: A second system is using the same system id (%1)").arg(getSystemId()));
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
                        emit protocolStatusMessage(tr("The MAVLink protocol version on the MAV and QGroundControl mismatch!"),
                                                   tr("It is unsafe to use different MAVLink versions. QGroundControl therefore refuses to connect to system %1, which sends MAVLink version %2 (QGroundControl uses version %3).").arg(message.sysid).arg(heartbeat.mavlink_version).arg(MAVLINK_VERSION));
                        versionMismatchIgnore = true;
                    }

                    // Ignore this message and continue gracefully
                    continue;
                }

                // Create a new UAS object
                uas = QGCMAVLinkUASFactory::createUAS(this, link, message.sysid, &heartbeat);

            }

            // Increase receive counter
            totalReceiveCounter[linkId]++;
            currReceiveCounter[linkId]++;

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
                totalLossCounter[linkId] += lostMessages;
                currLossCounter[linkId] += lostMessages;
            }

            // And update the last sequence number for this system/component pair
            lastIndex[message.sysid][message.compid] = expectedSeq;

            // Update on every 32th packet
            if ((totalReceiveCounter[linkId] & 0x1F) == 0)
            {
                // Calculate new loss ratio
                // Receive loss
                float receiveLoss = (double)currLossCounter[linkId]/(double)(currReceiveCounter[linkId]+currLossCounter[linkId]);
                receiveLoss *= 100.0f;
                currLossCounter[linkId] = 0;
                currReceiveCounter[linkId] = 0;
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
                QList<LinkInterface*> links = LinkManager::instance()->getLinksForProtocol(this);

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
    QList<LinkInterface*> links = LinkManager::instance()->getLinksForProtocol(this);

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
    if (link->getId() != 0) mavlink_finalize_message_chan(&message, this->getSystemId(), this->getComponentId(), link->getId(), message.len, messageKeys[message.msgid]);
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
    if (link->getId() != 0) mavlink_finalize_message_chan(&message, systemid, componentid, link->getId(), message.len, messageKeys[message.msgid]);
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
    if (m_heartbeatsEnabled)
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

/** @param enabled true to enable heartbeats emission at heartbeatRate, false to disable */
void MAVLinkProtocol::enableHeartbeats(bool enabled)
{
    m_heartbeatsEnabled = enabled;
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

void MAVLinkProtocol::enableLogging(bool enabled)
{
    bool changed = false;
    if (enabled != m_loggingEnabled) changed = true;

    if (enabled)
    {
        if (m_logfile && m_logfile->isOpen())
        {
            m_logfile->flush();
            m_logfile->close();
        }

        if (m_logfile)
        {
            if (!m_logfile->open(QIODevice::WriteOnly | QIODevice::Append))
            {
                emit protocolStatusMessage(tr("Opening MAVLink logfile for writing failed"), tr("MAVLink cannot log to the file %1, please choose a different file. Stopping logging.").arg(m_logfile->fileName()));
                m_loggingEnabled = false;
            }
        }
        else
        {
            emit protocolStatusMessage(tr("Opening MAVLink logfile for writing failed"), tr("MAVLink cannot start logging, no logfile selected."));
        }
    }
    else if (!enabled)
    {
        if (m_logfile)
        {
            if (m_logfile->isOpen())
            {
                m_logfile->flush();
                m_logfile->close();
            }
        }
    }
    m_loggingEnabled = enabled;
    if (changed) emit loggingChanged(enabled);
}

void MAVLinkProtocol::setLogfileName(const QString& filename)
{
    if (!m_logfile)
    {
        m_logfile = new QFile(filename);
    }
    else
    {
        m_logfile->flush();
        m_logfile->close();
    }
    m_logfile->setFileName(filename);
    enableLogging(m_loggingEnabled);
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
    heartbeatRate = rate;
    heartbeatTimer->setInterval(1000/heartbeatRate);
}

/** @return heartbeat rate in Hertz */
int MAVLinkProtocol::getHeartbeatRate()
{
    return heartbeatRate;
}
