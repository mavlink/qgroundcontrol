/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#pragma once

#include "FileManager.h"

#include <QStringList>

class MockLink;

/// Mock implementation of Mavlink FTP server.
class MockLinkFileServer : public QObject
{
    Q_OBJECT
    
public:
    MockLinkFileServer(uint8_t systemIdServer, uint8_t componentIdServer, MockLink* mockLink);
    
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
    
    /// Called to handle an FTP message
    void handleFTPMessage(const mavlink_message_t& message);
    
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
    
    void enableRandromDrops(bool enable) { _randomDropsEnabled = enable; }

signals:
    /// You can connect to this signal to be notified when the server receives a Terminate command.
    void terminateCommandReceived(void);
    
    /// You can connect to this signal to be notified when the server receives a Reset command.
    void resetCommandReceived(void);
    
private:
	void _sendAck(uint8_t targetSystemId, uint8_t targetComponentId, uint16_t seqNumber, FileManager::Opcode reqOpcode);
    void _sendNak(uint8_t targetSystemId, uint8_t targetComponentId, FileManager::ErrorCode error, uint16_t seqNumber, FileManager::Opcode reqOpcode);
    void _sendResponse(uint8_t targetSystemId, uint8_t targetComponentId, FileManager::Request* request, uint16_t seqNumber);
    void _listCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber);
    void _openCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber);
    void _readCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber);
	void _streamCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber);
    void _terminateCommand(uint8_t senderSystemId, uint8_t senderComponentId, FileManager::Request* request, uint16_t seqNumber);
    void _resetCommand(uint8_t senderSystemId, uint8_t senderComponentId, uint16_t seqNumber);
    uint16_t _nextSeqNumber(uint16_t seqNumber);
    
    /// if request is a string, this ensures it's null-terminated
    static void ensureNullTemination(FileManager::Request* request);

    QStringList _fileList;  ///< List of files returned by List command
    
    static const uint8_t    _sessionId;
    uint8_t                 _readFileLength;    ///< Length of active file being read
    ErrorMode_t             _errMode;           ///< Currently set error mode, as specified by setErrorMode
    const uint8_t           _systemIdServer;    ///< System ID for server
    const uint8_t           _componentIdServer; ///< Component ID for server
    MockLink*               _mockLink;          ///< MockLink to communicate through

    bool _lastReplyValid;
    uint16_t _lastReplySequence;
    mavlink_message_t _lastReply;

    bool _randomDropsEnabled;
};

