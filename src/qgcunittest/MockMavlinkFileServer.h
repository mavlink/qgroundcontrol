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

#ifndef MOCKMAVLINKFILESERVER_H
#define MOCKMAVLINKFILESERVER_H

#include "MockMavlinkInterface.h"
#include "QGCUASFileManager.h"

/// @file
///     @brief Mock implementation of Mavlink FTP server. Used as mavlink plugin to MockUAS.
///             Only root directory access is supported.
///
///     @author Don Gagne <don@thegagnes.com>

#include <QStringList>

class MockMavlinkFileServer : public MockMavlinkInterface
{
    Q_OBJECT
    
public:
    MockMavlinkFileServer(void);
    
    /// @brief Sets the list of files returned by the List command. Prepend names with F or D
    /// to indicate (F)ile or (D)irectory.
    void setFileList(QStringList& fileList) { _fileList = fileList; }
    
    typedef enum {
        errModeNone,                ///< No error, respond correctly
        errModeNoResponse,          ///< No response to any request
        errModeNakResponse,         ///< Nak all requests
        errModeNoSecondResponse,    ///< No response to subsequent request to initial command
        errModeNakSecondResponse,   ///< Nak subsequent request to initial command
        errModeBadCRC,              ///< Return response with bad CRC
        errModeBadSequence          ///< Return response with bad sequence number, NYI: Waiting on Firmware sequence # support
    } ErrorMode_t;
    
    ///< Array of failure modes you can cycle through for testing
    static const ErrorMode_t rgFailureModes[];
    static const size_t cFailureModes;
    
    /// @brief Sets the error mode for command responses.
    void setErrorMode(ErrorMode_t errMode) { _errMode = errMode; };
    
    // From MockMavlinkInterface
    virtual void sendMessage(mavlink_message_t message);
    
    struct FileTestCase {
        const char* filename;
        uint8_t     length;
        bool        fMultiPacketResponse;   ///< true: multiple acks required to download, false: single ack contains entire download
    };
    
    static const size_t cFileTestCases = 3;
    static const FileTestCase rgFileTestCases[cFileTestCases];
    
signals:
    void terminateCommandReceived(void);
    
private:
    void _sendAck(void);
    void _sendNak(QGCUASFileManager::ErrorCode error);
    void _emitResponse(QGCUASFileManager::Request* request);
    void _listCommand(QGCUASFileManager::Request* request);
    void _openCommand(QGCUASFileManager::Request* request);
    void _readCommand(QGCUASFileManager::Request* request);
    void _terminateCommand(QGCUASFileManager::Request* request);
    
    QStringList _fileList;  ///< List of files returned by List command
    
    static const uint8_t    _sessionId;
    uint8_t                 _readFileLength; ///< Length of active file being read
    ErrorMode_t             _errMode;
};

#endif
