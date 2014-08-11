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
    QGCUASFileManager(QObject* parent, UASInterface* uas);
    
    /// These methods are only used for testing purposes.
    bool _sendCmdTestAck(void) { return _sendOpcodeOnlyCmd(kCmdNone, kCOAck); };
    bool _sendCmdTestNoAck(void) { return _sendOpcodeOnlyCmd(kCmdTestNoAck, kCOAck); };
    bool _sendCmdReset(void) { return _sendOpcodeOnlyCmd(kCmdReset, kCOAck); };
    
    /// @brief Timeout in msecs to wait for an Ack time come back. This is public so we can write unit tests which wait long enough
    /// for the FileManager to timeout.
    static const int ackTimerTimeoutMsecs = 1000;

signals:
    void statusMessage(const QString& msg);
    void resetStatusMessages();
    void errorMessage(const QString& msg);
    void listComplete(void);
    void openFileLength(unsigned int length);

public slots:
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
    void listDirectory(const QString& dirPath);
    void downloadPath(const QString& from, const QDir& downloadDir);

protected:
    static const uint8_t kProtocolMagic = 'f';
    struct RequestHeader
        {
            uint8_t		magic;      ///> Magic byte 'f' to idenitfy FTP protocol
            uint8_t		session;    ///> Session id for read and write commands
            uint8_t		opcode;     ///> Command opcode
            uint8_t		size;       ///> Size of data
            uint32_t	crc32;      ///> CRC for entire Request structure, with crc32 set to 0
            uint32_t	offset;     ///> Offsets for List and Read commands
        };

    struct Request
    {
        struct RequestHeader hdr;

        // We use a union here instead of just casting (uint32_t)&data[0] to not break strict aliasing rules
        union {
            // The entire Request must fit into the data member of the mavlink_encapsulated_data_t structure. We use as many leftover bytes
            // after we use up space for the RequestHeader for the data portion of the Request.
            uint8_t data[sizeof(((mavlink_encapsulated_data_t*)0)->data) - sizeof(RequestHeader)];

            // File length returned by Open command
            uint32_t openFileLength;
        };
    };

    enum Opcode
        {
            // Commands
            kCmdNone,       ///> ignored, always acked
            kCmdTerminate,	///> releases sessionID, closes file
            kCmdReset,      ///> terminates all sessions
            kCmdList,       ///> list files in <path> from <offset>
            kCmdOpen,       ///> opens <path> for reading, returns <session>
            kCmdRead,       ///> reads <size> bytes from <offset> in <session>
            kCmdCreate,     ///> creates <path> for writing, returns <session>
            kCmdWrite,      ///> appends <size> bytes at <offset> in <session>
            kCmdRemove,     ///> remove file (only if created by server?)

            // Responses
            kRspAck,        ///> positive acknowledgement of previous command
            kRspNak,        ///> negative acknowledgement of previous command
            
            // Used for testing only, not part of protocol
            kCmdTestNoAck,  // ignored, ack not sent back, should timeout waiting for ack
        };

    enum ErrorCode
        {
            kErrNone,
            kErrNoRequest,
            kErrNoSession,
            kErrSequence,
            kErrNotDir,
            kErrNotFile,
            kErrEOF,
            kErrNotAppend,
            kErrTooBig,
            kErrIO,
            kErrPerm,
            kErrUnknownCommand,
            kErrCrc
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
    void _emitStatusMessage(const QString& msg);
    void _sendRequest(Request* request);
    void _fillRequestWithString(Request* request, const QString& str);
    void _openAckResponse(Request* openAck);
    void _readAckResponse(Request* readAck);
    void _listAckResponse(Request* listAck);
    void _sendListCommand(void);
    void _sendTerminateCommand(void);
    void _closeReadSession(bool success);
    
    static quint32 crc32(Request* request, unsigned state = 0);
    static QString errorString(uint8_t errorCode);

    OperationState  _currentOperation;              ///> Current operation of state machine
    QTimer          _ackTimer;                      ///> Used to signal a timeout waiting for an ack
    
    UASInterface* _mav;
    
    uint16_t _lastOutgoingSeqNumber; ///< Sequence number sent in last outgoing packet

    unsigned    _listOffset;    ///> offset for the current List operation
    QString     _listPath;      ///> path for the current List operation
    
    uint8_t     _activeSession;             ///> currently active session, 0 for none
    uint32_t    _readOffset;                ///> current read offset
    QByteArray  _readFileAccumulator;       ///> Holds file being downloaded
    QDir        _readFileDownloadDir;       ///> Directory to download file to
    QString     _readFileDownloadFilename;  ///> Filename (no path) for download file
    
    // We give MockMavlinkFileServer friend access so that it can use the data structures and opcodes
    // to build a mock mavlink file server for testing.
    friend class MockMavlinkFileServer;
};

#endif // QGCUASFILEMANAGER_H
