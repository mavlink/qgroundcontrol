/*!
 *   @brief Typhoon H QGCCorePlugin Implementation
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#include "TlogParser.h"
#include "QGCApplication.h"
#include "LinkManager.h"

#define TIMESTAMP_SIZE sizeof(quint64)

//-----------------------------------------------------------------------------
TlogParser::TlogParser()
    : _curTimeUSecs(0)
    , _mavlinkChannel(0)
{

}

//-----------------------------------------------------------------------------
TlogParser::~TlogParser()
{
    if(_mavlinkChannel) {
        qgcApp()->toolbox()->linkManager()->_freeMavlinkChannel(_mavlinkChannel);
        _mavlinkChannel = 0;
    }
}

//-----------------------------------------------------------------------------
bool
TlogParser::parseLogFile(const QString& logFilename)
{
    if (_logFile.isOpen()) {
        qWarning() << "Attempting to parse an already open log file.";
        return false;
    }
    _logFile.setFileName(logFilename);
    if (!_logFile.open(QFile::ReadOnly)) {
        qWarning() << QString("Unable to open log file: '%1', error: %2").arg(logFilename).arg(_logFile.errorString());
        return false;
    }
    QByteArray timestamp = _logFile.read(TIMESTAMP_SIZE);
    _curTimeUSecs = _parseTimestamp(timestamp);
    //-- Parse log file
    if(!_mavlinkChannel) {
        _mavlinkChannel = qgcApp()->toolbox()->linkManager()->_reserveMavlinkChannel();
    }
    if (_mavlinkChannel == 0) {
        qWarning() << "No mavlink channels available";
        _logFile.close();
        return false;
    }
    this->start(QThread::HighPriority);
    return true;
}

//-----------------------------------------------------------------------------
void
TlogParser::run()
{
    while(true) {
        mavlink_message_t message;
        qint64 nextTimeUSecs = _readNextMavlinkMessage(message);
        if(!nextTimeUSecs) {
            break;
        }
        emit newMavlinkMessage(_curTimeUSecs, message);
        _curTimeUSecs = nextTimeUSecs;
    }
    emit completed();
}

//-----------------------------------------------------------------------------
quint64
TlogParser::_parseTimestamp(const QByteArray& bytes)
{
    quint64 timestamp = qFromBigEndian(*((quint64*)(bytes.constData())));
    quint64 currentTimestamp = ((quint64)QDateTime::currentMSecsSinceEpoch()) * 1000;
    if (timestamp > currentTimestamp) {
        timestamp = qbswap(timestamp);
    }
    return timestamp;
}

//-----------------------------------------------------------------------------
quint64
TlogParser::_readNextMavlinkMessage(mavlink_message_t& message)
{
    char                nextByte;
    mavlink_status_t    status;
    while (_logFile.getChar(&nextByte)) { // Loop over every byte
        bool messageFound = mavlink_parse_char(_mavlinkChannel, nextByte, &message, &status);
        if (messageFound) {
            // Return the timestamp for the next message
            QByteArray rawTime = _logFile.read(TIMESTAMP_SIZE);
            return _parseTimestamp(rawTime);
        }
    }
    return 0;
}
