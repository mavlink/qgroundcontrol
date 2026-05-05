#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "MAVLinkFTP.h"

class MockLink;

/// \brief Mock implementation of Mavlink FTP server.
///
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

    void enableRandomDrops(bool enable) { _randomDropsEnabled = enable; }

    /// Drops the next ResetSessions responses after applying the reset on the server.
    void setResetCommandResponseDropCount(int count) { _resetCommandResponseDropCount = count; }

    /// Returns the list of remote paths which have been uploaded in this session.
    QStringList uploadedFiles() const { return _uploadedFiles.keys(); }

    /// Returns the contents of an uploaded file. Empty if the path is unknown.
    QByteArray uploadedFileContents(const QString& remotePath) const { return _uploadedFiles.value(remotePath); }

    /// Clears the stored uploaded file contents.
    void clearUploadedFiles() { _uploadedFiles.clear(); }

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

    /// Controls whether the server implements timestamped directory listings. When false the server
    /// returns MAV_FTP_ERR_UNKNOWNCOMMAND so the client's plain-listing fallback can be tested.
    void setListDirectoryWithTimeSupported(bool supported) { _listDirectoryWithTimeSupported = supported; }

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

    /// Base modification time (seconds since UNIX epoch UTC) reported by timestamped directory listings.
    /// mock listing. Entry N reports kMockModificationTime + N.
    static constexpr uint32_t kMockModificationTime = 1700000000;

signals:
    /// You can connect to this signal to be notified when the server receives a Terminate command.
    void terminateCommandReceived();

    /// You can connect to this signal to be notified when the server receives a Reset command.
    void resetCommandReceived();

private:
    /// Sends an Ack
    void _sendAck(uint8_t targetSystemId, uint8_t targetComponentId, uint16_t seqNumber, MAV_FTP_OPCODE reqOpCode);
    void _sendNak(uint8_t targetSystemId, uint8_t targetComponentId, MAV_FTP_ERR error, uint16_t seqNumber, MAV_FTP_OPCODE reqOpCode);
    void _sendNakErrno(uint8_t targetSystemId, uint8_t targetComponentId, uint8_t nakErrno, uint16_t seqNumber, MAV_FTP_OPCODE reqOpCode);
    /// Emits a Request through the messageReceived signal.
    void _sendResponse(uint8_t targetSystemId, uint8_t targetComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    /// Handles List command requests. Only supports root folder paths.
    /// File list returned is set using the setFileList method.
    void _listCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber, bool withTime);
    void _openCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _createFileCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _openFileWOCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _readCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _burstReadCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _terminateCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _resetCommand(uint8_t senderSystemId, uint8_t senderComponentId, uint16_t seqNumber);
    void _writeCommand(uint8_t senderSystemId, uint8_t senderComponentId, MavlinkFTP::Request *request, uint16_t seqNumber);
    void _finalizeActiveUpload();
    /// Generates the next sequence number given an incoming sequence number. Handles generating
    /// bad sequence numbers when errModeBadSequence is set.
    uint16_t _nextSeqNumber(uint16_t seqNumber) const;
    static QString _createTestTempFile(int size);
    QString _generateParamPck(bool withDefaults);

    /// if request is a string, this ensures it's null-terminated
    static void ensureNullTemination(MavlinkFTP::Request *request);

    const uint8_t _systemIdServer;              ///< System ID for server
    const uint8_t _componentIdServer;           ///< Component ID for server
    MockLink *_mockLink;                        ///< MockLink to communicate through

    bool _lastReplyValid = false;
    bool _randomDropsEnabled = false;
    int _resetCommandResponseDropCount = 0;
    ErrorMode_t _errMode = errModeNone;         ///< Currently set error mode, as specified by setErrorMode
    bool _listDirectoryWithTimeSupported = true; ///< Whether the server implements timestamped listings.
    mavlink_message_t _lastReply{};
    QFile _currentFile;
    QString _paramPckTempFile;
    struct UploadSession {
        bool active = false;
        QString remotePath;
        QByteArray buffer;

        void reset() {
            active = false;
            remotePath.clear();
            buffer.clear();
        }
    };
    UploadSession _uploadSession;
    QHash<QString, QByteArray> _uploadedFiles;
    QStringList _fileList;                      ///< List of files returned by List command
    uint16_t _lastReplySequence = 0;

    static constexpr uint8_t _sessionId = 1;    ///< We only support a single fixed session
};
