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

signals:
    void statusMessage(const QString& msg);
    void resetStatusMessages();
    void errorMessage(const QString& ms);

public slots:
    void receiveMessage(LinkInterface* link, mavlink_message_t message);
    void nothingMessage();
    void listRecursively(const QString &from);
    void downloadPath(const QString& from, const QString& to);

protected:
    struct RequestHeader
        {
            uint8_t		magic;
            uint8_t		session;
            uint8_t		opcode;
            uint8_t		size;
            uint32_t	crc32;
            uint32_t	offset;
        };

    struct Request
    {
        struct RequestHeader hdr;
        // The entire Request must fit into the data member of the mavlink_encapsulated_data_t structure. We use as many leftover bytes
        // after we use up space for the RequestHeader for the data portion of the Request.
        uint8_t data[sizeof(((mavlink_encapsulated_data_t*)0)->data) - sizeof(RequestHeader)];
    };

    enum Opcode
        {
            kCmdNone,       // ignored, always acked
            kCmdTerminate,	// releases sessionID, closes file
            kCmdReset,      // terminates all sessions
            kCmdList,       // list files in <path> from <offset>
            kCmdOpen,       // opens <path> for reading, returns <session>
            kCmdRead,       // reads <size> bytes from <offset> in <session>
            kCmdCreate,     // creates <path> for writing, returns <session>
            kCmdWrite,      // appends <size> bytes at <offset> in <session>
            kCmdRemove,     // remove file (only if created by server?)

            kRspAck,
            kRspNak,
            
            kCmdTestNoAck,  // ignored, ack not sent back, for testing only, should timeout waiting for ack
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
            kCOList,    // waiting for a List response
        };
    
    
protected slots:
    void _ackTimeout(void);
    
protected:
    bool _sendOpcodeOnlyCmd(uint8_t opcode, OperationState newOpState);
    void _setupAckTimeout(void);
    void _clearAckTimeout(void);
    void _emitErrorMessage(const QString& msg);
    void _sendRequest(Request* request);

    void sendList();
    void listDecode(const uint8_t *data, unsigned len);

    static quint32 crc32(Request* request, unsigned state = 0);
    static QString errorString(uint8_t errorCode);

    OperationState      _currentOperation;              ///> Current operation of state machine
    QTimer              _ackTimer;                      ///> Used to signal a timeout waiting for an ack
    static const int    _ackTimerTimeoutMsecs = 1000;   ///> Timeout in msecs for ack timer
    
    UASInterface* _mav;
    quint16 _encdata_seq;

    unsigned    _listOffset;    // offset for the current List operation
    QString     _listPath;      // path for the current List operation
    
    // We give MockMavlinkFileServer friend access so that it can use the data structures and opcodes
    // to build a mock mavlink file server for testing.
    friend class MockMavlinkFileServer;
};

#endif // QGCUASFILEMANAGER_H
