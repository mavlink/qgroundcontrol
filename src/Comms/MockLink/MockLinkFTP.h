/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "MAVLinkFTP.h"

Q_DECLARE_LOGGING_CATEGORY(MockLinkFTPLog)

class MockLink;

/// Mock implementation of Mavlink FTP server.
class MockLinkFTP : public QObject
{
    Q_OBJECT

public:
    MockLinkFTP(uint8_t systemIdServer, uint8_t componentIdServer, MockLink *mockLink);
    ~MockLinkFTP();

    /// Sets the list of files returned by the List command. Prepend names with F or D
    /// to indicate (F)ile or (D)irectory.
    void setFileList(const QStringList &fileList) { _fileList = fileList; }

    /// Called to handle an FTP message
    void mavlinkMessageReceived(const mavlink_message_t &message);

    void enableRandromDrops(bool enable) { _randomDropsEnabled = enable; }
    void enableBinParamFile(bool enable) { _BinParamFileEnabled = enable; }

    /// By calling setErrorMode with one of these modes you can cause the server to simulate an error.
    enum ErrorMode_t {
        errModeNone,                        ///< No error, respond correctly
        errModeNoResponse,                  ///< No response to any request, client should eventually time out with no Ack
        errModeNakResponse,                 ///< Nak all requests
        errModeNoSecondResponse,            ///< No response to subsequent request to initial command
        errModeNoSecondResponseAllowRetry,  ///< No response to subsequent request to initial command, error will be cleared after this so retry will succeed
        errModeNakSecondResponse,           ///< Nak subsequent request to initial command
        errModeBadSequence                  ///< Return response with bad sequence number
    };

    /// Sets the error mode for command responses. This allows you to simulate various server errors.
    void setErrorMode(ErrorMode_t errMode) { _errMode = errMode; };

    /// Array of failure modes you can cycle through for testing. By looping through this array you can avoid
    /// hardcoding the specific error modes in your unit test. This way when new error modes are added your unit test
    /// code may not need to be modified.
    static constexpr const ErrorMode_t rgFailureModes[] = {
        errModeNoResponse,
        errModeNakResponse,
        errModeNoSecondResponse,
        errModeNakSecondResponse,
        errModeBadSequence,
    };

    /// The number of ErrorModes in the rgFailureModes array.
    static constexpr const size_t cFailureModes = std::size(MockLinkFTP::rgFailureModes);

    static constexpr const char *sizeFilenamePrefix = "mocklink-size-";

signals:
    /// You can connect to this signal to be notified when the server receives a Terminate command.
    void terminateCommandReceived();

    /// You can connect to this signal to be notified when the server receives a Reset command.
    void resetCommandReceived();

private:
    /// Sends an Ack
    void _sendAck(uint8_t targetSystemId, uint8_t targetComponentId, uint16_t seqNumber, MavlinkFTP::OpCode_t reqOpCode);
    void _sendNak(uint8_t targetSystemId, uint8_t targetComponentId, MavlinkFTP::ErrorCode_t error, uint16_t seqNumber, MavlinkFTP::OpCode_t reqOpCode);
    void _sendNakErrno(uint8_t targetSystemId, uint8_t targetComponentId, uint8_t nakErrno, uint16_t seqNumber, MavlinkFTP::OpCode_t reqOpCode);
    /// Emits a Request through the messageReceived signal.
    void _sendResponse(uint8_t targetSystemId, uint8_t targetComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    /// Handles List command requests. Only supports root folder paths.
    /// File list returned is set using the setFileList method.
    void _listCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _openCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _readCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _burstReadCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _terminateCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _resetCommand(uint8_t senderSystemId, uint8_t senderComponentId, uint16_t seqNumber);
    /// Generates the next sequence number given an incoming sequence number. Handles generating
    /// bad sequence numbers when errModeBadSequence is set.
    uint16_t _nextSeqNumber(uint16_t seqNumber) const;
    static QString _createTestTempFile(int size);

    /// if request is a string, this ensures it's null-terminated
    static void ensureNullTemination(MavlinkFTP::Request *request);

    const uint8_t _systemIdServer;              ///< System ID for server
    const uint8_t _componentIdServer;           ///< Component ID for server
    MockLink *_mockLink;                        ///< MockLink to communicate through

    bool _BinParamFileEnabled = false;
    bool _lastReplyValid = false;
    bool _randomDropsEnabled = false;
    ErrorMode_t _errMode = errModeNone;         ///< Currently set error mode, as specified by setErrorMode
    mavlink_message_t _lastReply{};
    QFile _currentFile;
    QStringList _fileList;                      ///< List of files returned by List command
    uint16_t _lastReplySequence = 0;

    static constexpr uint8_t _sessionId = 1;    ///< We only support a single fixed session
};

