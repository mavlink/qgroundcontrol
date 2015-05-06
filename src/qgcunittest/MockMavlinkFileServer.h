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
#include "FileManager.h"

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
    /// @brief Constructor for MockMavlinkFileServer
    ///     @param System ID for QGroundControl App
    ///     @pqram System ID for this Server
    MockMavlinkFileServer(uint8_t systemIdQGC, uint8_t systemIdServer);
    
    /// @brief Sets the list of files returned by the List command. Prepend names with F or D
    /// to indicate (F)ile or (D)irectory.
    void setFileList(QStringList& fileList) { _fileList = fileList; }
    
    /// @brief By calling setErrorMode with one of these modes you can cause the server to simulate an error.
    typedef enum {
        errModeNone,                ///< No error, respond correctly
        errModeNoResponse,          ///< No response to any request, client should eventually time out with no Ack
        errModeNakResponse,         ///< Nak all requests
        errModeNoSecondResponse,    ///< No response to subsequent request to initial command
        errModeNakSecondResponse,   ///< Nak subsequent request to initial command
        errModeBadSequence          ///< Return response with bad sequence number
    } ErrorMode_t;
    
    /// @brief Sets the error mode for command responses. This allows you to simulate various server errors.
    void setErrorMode(ErrorMode_t errMode) { _errMode = errMode; };
    
    /// @brief Array of failure modes you can cycle through for testing. By looping through this array you can avoid
    /// hardcoding the specific error modes in your unit test. This way when new error modes are added your unit test
    /// code may not need to be modified.
    static const ErrorMode_t rgFailureModes[];
    
    /// @brief The number of ErrorModes in the rgFailureModes array.
    static const size_t cFailureModes;
    
    // From MockMavlinkInterface
    virtual void sendMessage(mavlink_message_t message);
    
    /// @brief Used to represent a single test case for download testing.
    struct FileTestCase {
        const char* filename;               ///< Filename to download
        uint8_t     length;                 ///< Length of file in bytes
		int			packetCount;			///< Number of packets required for data
        bool        exactFit;				///< true: last packet is exact fit, false: last packet is partially filled
    };
    
    /// @brief The numbers of test cases in the rgFileTestCases array.
    static const size_t cFileTestCases = 3;
    
    /// @brief The set of files supported by the mock server for testing purposes. Each one represents a different edge case for testing.
    static const FileTestCase rgFileTestCases[cFileTestCases];
    
signals:
    /// @brief You can connect to this signal to be notified when the server receives a Terminate command.
    void terminateCommandReceived(void);
    
private:
	void _sendAck(uint16_t seqNumber, FileManager::Opcode reqOpcode);
    void _sendNak(FileManager::ErrorCode error, uint16_t seqNumber, FileManager::Opcode reqOpcode);
    void _emitResponse(FileManager::Request* request, uint16_t seqNumber);
    void _listCommand(FileManager::Request* request, uint16_t seqNumber);
    void _openCommand(FileManager::Request* request, uint16_t seqNumber);
    void _readCommand(FileManager::Request* request, uint16_t seqNumber);
	void _streamCommand(FileManager::Request* request, uint16_t seqNumber);
    void _terminateCommand(FileManager::Request* request, uint16_t seqNumber);
    uint16_t _nextSeqNumber(uint16_t seqNumber);
    
    QStringList _fileList;  ///< List of files returned by List command
    
    static const uint8_t    _sessionId;
    uint8_t                 _readFileLength;    ///< Length of active file being read
    ErrorMode_t             _errMode;           ///< Currently set error mode, as specified by setErrorMode
    const uint8_t           _systemIdServer;    ///< System ID for server
    const uint8_t           _systemIdQGC;       ///< QGC System ID
};

#endif
