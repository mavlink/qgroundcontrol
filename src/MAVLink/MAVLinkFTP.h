/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QString>
#include <QtCore/QLoggingCategory>

#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(MavlinkFTPLog)

class MavlinkFTP {
public:
    /// This is the fixed length portion of the protocol data.
    /// This needs to be packed, because it's typecasted from mavlink_file_transfer_protocol_t.payload, which starts
    /// at a 3 byte offset, causing an unaligned access to seq_number and offset
    MAVPACKED(
            typedef struct RequestHeader {
                uint16_t    seqNumber;      ///< sequence number for message
                uint8_t     session;        ///< Session id for read and write commands
                uint8_t     opcode;         ///< Command opcode
                uint8_t     size;           ///< Size of data
                uint8_t     req_opcode;     ///< Request opcode returned in kRspAck, kRspNak message
                uint8_t     burstComplete;  ///< Only used if req_opcode=kCmdBurstReadFile - 1: set of burst packets complete, 0: More burst packets coming.
                uint8_t     paddng;        ///< 32 bit aligment padding
                uint32_t    offset;         ///< Offsets for List and Read commands
            }) RequestHeader;

    MAVPACKED(
            typedef struct Request{
                RequestHeader hdr;

                // We use a union here instead of just casting (uint32_t)&payload[0] to not break strict aliasing rules
                union {
                    // The entire Request must fit into the payload member of the mavlink_file_transfer_protocol_t structure. We use as many leftover bytes
                    // after we use up space for the RequestHeader for the data portion of the Request.
                    uint8_t data[sizeof(((mavlink_file_transfer_protocol_t*)0)->payload) - sizeof(RequestHeader)];

                    // File length returned by Open command
                    uint32_t openFileLength;

                    // Length of file chunk written by write command
                    uint32_t writeFileLength;
                };
            }) Request;

    typedef enum {
        kCmdNone = 0,           ///< ignored, always acked
        kCmdTerminateSession,	///< Terminates open Read session
        kCmdResetSessions,		///< Terminates all open Read sessions
        kCmdListDirectory,		///< List files in <path> from <offset>
        kCmdOpenFileRO,			///< Opens file at <path> for reading, returns <session>
        kCmdReadFile,			///< Reads <size> bytes from <offset> in <session>
        kCmdCreateFile,			///< Creates file at <path> for writing, returns <session>
        kCmdWriteFile,			///< Writes <size> bytes to <offset> in <session>
        kCmdRemoveFile,			///< Remove file at <path>
        kCmdCreateDirectory,	///< Creates directory at <path>
        kCmdRemoveDirectory,	///< Removes Directory at <path>, must be empty
        kCmdOpenFileWO,			///< Opens file at <path> for writing, returns <session>
        kCmdTruncateFile,		///< Truncate file at <path> to <offset> length
        kCmdRename,				///< Rename <path1> to <path2>
        kCmdCalcFileCRC32,		///< Calculate CRC32 for file at <path>
        kCmdBurstReadFile,      ///< Burst download session file

        kRspAck = 128,          ///< Ack response
        kRspNak,                ///< Nak response
    } OpCode_t;

    /// @brief Error codes returned in Nak response PayloadHeader.data[0].
   typedef enum {
        kErrNone = 0,
        kErrFail,                   ///< Unknown failure
        kErrFailErrno,              ///< errno sent back in PayloadHeader.data[1]
        kErrInvalidDataSize,		///< PayloadHeader.size is invalid
        kErrInvalidSession,         ///< Session is not currently open
        kErrNoSessionsAvailable,	///< All available Sessions in use
        kErrEOF,                    ///< Offset past end of file for List and Read commands
        kErrUnknownCommand,         ///< Unknown command opcode
        kErrFailFileExists,         ///< File exists already
        kErrFailFileProtected,      ///< File is write protected
        kErrFailFileNotFound
    } ErrorCode_t;

    static QString opCodeToString   (OpCode_t opCode);
    static QString errorCodeToString(ErrorCode_t errorCode);
};
