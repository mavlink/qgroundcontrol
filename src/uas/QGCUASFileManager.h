/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#ifndef QGCUASFILEMANAGER_H
#define QGCUASFILEMANAGER_H

#include <QObject>
#include <QDir>

#include "UASInterface.h"

class QGCUASFileManager : public QObject
{
    Q_OBJECT
public:
    QGCUASFileManager(QObject* parent, UASInterface* uas, uint8_t unitTestSystemIdQGC = 0);
    
    /// These methods are only used for testing purposes.
    bool _sendCmdTestAck(void) { return _sendOpcodeOnlyCmd(kCmdNone, kCOAck); };
    bool _sendCmdTestNoAck(void) { return _sendOpcodeOnlyCmd(kCmdTestNoAck, kCOAck); };
    bool _sendCmdReset(void) { return _sendOpcodeOnlyCmd(kCmdResetSessions, kCOAck); };
    
    /// @brief Timeout in msecs to wait for an Ack time come back. This is public so we can write unit tests which wait long enough
    /// for the FileManager to timeout.
    static const int ackTimerTimeoutMsecs = 1000;

signals:
    /// @brief Signalled whenever an error occurs during the listDirectory or downloadPath methods.
    void errorMessage(const QString& msg);
    
    // Signals associated with the listDirectory method
    
    /// @brief Signalled to indicate a new directory entry was received.
    void listEntry(const QString& entry);
    
    /// @brief Signalled after listDirectory completes. If an error occurs during directory listing this signal will not be emitted.
    void listComplete(void);
    
    // Signals associated with the downloadPath method
    
    /// @brief Signalled after downloadPath is called to indicate length of file being downloaded
    void downloadFileLength(unsigned int length);
    
    /// @brief Signalled during file download to indicate download progress
    ///     @param bytesReceived Number of bytes currently received from file
    void downloadFileProgress(unsigned int bytesReceived);
    
    /// @brief Signaled to indicate completion of file download. If an error occurs during download this signal will not be emitted.
    void downloadFileComplete(void);

public slots:
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
    void listDirectory(const QString& dirPath);
    void downloadPath(const QString& from, const QDir& downloadDir);

protected:
    
    /// @brief This is the fixed length portion of the protocol data. Trying to pack structures across differing compilers is
    /// questionable, so we pad the structure ourselves to 32 bit alignment which should get us what we want.
    struct RequestHeader
        {
            uint16_t    seqNumber;  ///< sequence number for message
            uint8_t     session;    ///< Session id for read and write commands
            uint8_t     opcode;     ///< Command opcode
            uint8_t     size;       ///< Size of data
            uint8_t     req_opcode; ///< Request opcode returned in kRspAck, kRspNak message
            uint8_t     padding[2]; ///< 32 bit aligment padding
            uint32_t    offset;     ///< Offsets for List and Read commands
        };

    struct Request
    {
        struct RequestHeader hdr;

        // We use a union here instead of just casting (uint32_t)&payload[0] to not break strict aliasing rules
        union {
            // The entire Request must fit into the payload member of the mavlink_file_transfer_protocol_t structure. We use as many leftover bytes
            // after we use up space for the RequestHeader for the data portion of the Request.
            uint8_t data[sizeof(((mavlink_file_transfer_protocol_t*)0)->payload) - sizeof(RequestHeader)];

            // File length returned by Open command
            uint32_t openFileLength;
        };
    };

    enum Opcode
	{
		kCmdNone,               ///< ignored, always acked
		kCmdTerminateSession,	///< Terminates open Read session
		kCmdResetSessions,      ///< Terminates all open Read sessions
		kCmdListDirectory,      ///< List files in <path> from <offset>
		kCmdOpenFile,           ///< Opens file at <path> for reading, returns <session>
		kCmdReadFile,           ///< Reads <size> bytes from <offset> in <session>
		kCmdCreateFile,         ///< Creates file at <path> for writing, returns <session>
		kCmdWriteFile,          ///< Appends <size> bytes to file in <session>
		kCmdRemoveFile,         ///< Remove file at <path>
		kCmdCreateDirectory,	///< Creates directory at <path>
		kCmdRemoveDirectory,	///< Removes Directory at <path>, must be empty
		
		kRspAck = 128,          ///< Ack response
		kRspNak,                ///< Nak response

        // Used for testing only, not part of protocol
        kCmdTestNoAck,          ///< ignored, ack not sent back, should timeout waiting for ack
	};
	
	/// @brief Error codes returned in Nak response PayloadHeader.data[0].
	enum ErrorCode
    {
		kErrNone,
		kErrFail,                   ///< Unknown failure
		kErrFailErrno,              ///< errno sent back in PayloadHeader.data[1]
		kErrInvalidDataSize,		///< PayloadHeader.size is invalid
		kErrInvalidSession,         ///< Session is not currently open
		kErrNoSessionsAvailable,	///< All available Sessions in use
		kErrEOF,                    ///< Offset past end of file for List and Read commands
		kErrUnknownCommand          ///< Unknown command opcode
    };

    enum OperationState
        {
            kCOIdle,    // not doing anything
            kCOAck,     // waiting for an Ack
            kCOList,    // waiting for List response
            kCOOpen,    // waiting for Open response
            kCORead,    // waiting for Read response
        };
    
    
protected slots:
    void _ackTimeout(void);
    
protected:
    bool _sendOpcodeOnlyCmd(uint8_t opcode, OperationState newOpState);
    void _setupAckTimeout(void);
    void _clearAckTimeout(void);
    void _emitErrorMessage(const QString& msg);
    void _emitListEntry(const QString& entry);
    void _sendRequest(Request* request);
    void _fillRequestWithString(Request* request, const QString& str);
    void _openAckResponse(Request* openAck);
    void _readAckResponse(Request* readAck);
    void _listAckResponse(Request* listAck);
    void _sendListCommand(void);
    void _sendTerminateCommand(void);
    void _closeReadSession(bool success);
    
    static QString errorString(uint8_t errorCode);

    OperationState  _currentOperation;              ///< Current operation of state machine
    QTimer          _ackTimer;                      ///< Used to signal a timeout waiting for an ack
    
    UASInterface* _mav;
    
    uint16_t _lastOutgoingSeqNumber; ///< Sequence number sent in last outgoing packet

    unsigned    _listOffset;    ///< offset for the current List operation
    QString     _listPath;      ///< path for the current List operation
    
    uint8_t     _activeSession;             ///< currently active session, 0 for none
    uint32_t    _readOffset;                ///< current read offset
    QByteArray  _readFileAccumulator;       ///< Holds file being downloaded
    QDir        _readFileDownloadDir;       ///< Directory to download file to
    QString     _readFileDownloadFilename;  ///< Filename (no path) for download file
    
    uint8_t     _systemIdQGC;               ///< System ID for QGC
    uint8_t     _systemIdServer;            ///< System ID for server
    
    // We give MockMavlinkFileServer friend access so that it can use the data structures and opcodes
    // to build a mock mavlink file server for testing.
    friend class MockMavlinkFileServer;
};

#endif // QGCUASFILEMANAGER_H
