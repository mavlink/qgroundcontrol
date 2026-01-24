#include "MavlinkFTPTest.h"
#include "MAVLinkFTP.h"

#include <QtTest/QTest>

// ============================================================================
// OpCode String Tests - Individual Opcodes
// ============================================================================

void MavlinkFTPTest::_testOpCodeNone()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdNone), QStringLiteral("None"));
}

void MavlinkFTPTest::_testOpCodeTerminateSession()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdTerminateSession), QStringLiteral("Terminate Session"));
}

void MavlinkFTPTest::_testOpCodeResetSessions()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdResetSessions), QStringLiteral("Reset Sessions"));
}

void MavlinkFTPTest::_testOpCodeListDirectory()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdListDirectory), QStringLiteral("List Directory"));
}

void MavlinkFTPTest::_testOpCodeOpenFileRO()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdOpenFileRO), QStringLiteral("Open File RO"));
}

void MavlinkFTPTest::_testOpCodeReadFile()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdReadFile), QStringLiteral("Read File"));
}

void MavlinkFTPTest::_testOpCodeCreateFile()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdCreateFile), QStringLiteral("Create File"));
}

void MavlinkFTPTest::_testOpCodeWriteFile()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdWriteFile), QStringLiteral("Write File"));
}

void MavlinkFTPTest::_testOpCodeRemoveFile()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdRemoveFile), QStringLiteral("Remove File"));
}

void MavlinkFTPTest::_testOpCodeCreateDirectory()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdCreateDirectory), QStringLiteral("Create Directory"));
}

void MavlinkFTPTest::_testOpCodeRemoveDirectory()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdRemoveDirectory), QStringLiteral("Remove Directory"));
}

void MavlinkFTPTest::_testOpCodeOpenFileWO()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdOpenFileWO), QStringLiteral("Open File WO"));
}

void MavlinkFTPTest::_testOpCodeTruncateFile()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdTruncateFile), QStringLiteral("Truncate File"));
}

void MavlinkFTPTest::_testOpCodeRename()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdRename), QStringLiteral("Rename"));
}

void MavlinkFTPTest::_testOpCodeCalcFileCRC32()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdCalcFileCRC32), QStringLiteral("Calc File CRC32"));
}

void MavlinkFTPTest::_testOpCodeBurstReadFile()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kCmdBurstReadFile), QStringLiteral("Burst Read File"));
}

void MavlinkFTPTest::_testOpCodeAck()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kRspAck), QStringLiteral("Ack"));
}

void MavlinkFTPTest::_testOpCodeNak()
{
    QCOMPARE(MavlinkFTP::opCodeToString(MavlinkFTP::kRspNak), QStringLiteral("Nak"));
}

void MavlinkFTPTest::_testOpCodeUnknown()
{
    const auto unknownOpCode = static_cast<MavlinkFTP::OpCode_t>(255);
    QCOMPARE(MavlinkFTP::opCodeToString(unknownOpCode), QStringLiteral("Unknown OpCode"));

    const auto anotherUnknown = static_cast<MavlinkFTP::OpCode_t>(50);
    QCOMPARE(MavlinkFTP::opCodeToString(anotherUnknown), QStringLiteral("Unknown OpCode"));
}

// ============================================================================
// ErrorCode String Tests - Individual Error Codes
// ============================================================================

void MavlinkFTPTest::_testErrorCodeNone()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrNone), QStringLiteral("None"));
}

void MavlinkFTPTest::_testErrorCodeFail()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrFail), QStringLiteral("Fail"));
}

void MavlinkFTPTest::_testErrorCodeFailErrno()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrFailErrno), QStringLiteral("Fail Errorno"));
}

void MavlinkFTPTest::_testErrorCodeInvalidDataSize()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrInvalidDataSize), QStringLiteral("Invalid Data Size"));
}

void MavlinkFTPTest::_testErrorCodeInvalidSession()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrInvalidSession), QStringLiteral("Invalid Session"));
}

void MavlinkFTPTest::_testErrorCodeNoSessionsAvailable()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrNoSessionsAvailable), QStringLiteral("No Sessions Available"));
}

void MavlinkFTPTest::_testErrorCodeEOF()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrEOF), QStringLiteral("EOF"));
}

void MavlinkFTPTest::_testErrorCodeUnknownCommand()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrUnknownCommand), QStringLiteral("Unknown Command"));
}

void MavlinkFTPTest::_testErrorCodeFileExists()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrFailFileExists), QStringLiteral("File Already Exists"));
}

void MavlinkFTPTest::_testErrorCodeFileProtected()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrFailFileProtected), QStringLiteral("File Protected"));
}

void MavlinkFTPTest::_testErrorCodeFileNotFound()
{
    QCOMPARE(MavlinkFTP::errorCodeToString(MavlinkFTP::kErrFailFileNotFound), QStringLiteral("File Not Found"));
}

void MavlinkFTPTest::_testErrorCodeUnknown()
{
    const auto unknownError = static_cast<MavlinkFTP::ErrorCode_t>(255);
    QCOMPARE(MavlinkFTP::errorCodeToString(unknownError), QStringLiteral("Unknown Error"));

    const auto anotherUnknown = static_cast<MavlinkFTP::ErrorCode_t>(100);
    QCOMPARE(MavlinkFTP::errorCodeToString(anotherUnknown), QStringLiteral("Unknown Error"));
}

// ============================================================================
// Protocol Constant Tests
// ============================================================================

void MavlinkFTPTest::_testOpCodeEnumValues()
{
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdNone), 0);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdTerminateSession), 1);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdResetSessions), 2);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdListDirectory), 3);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdOpenFileRO), 4);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdReadFile), 5);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdCreateFile), 6);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdWriteFile), 7);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdRemoveFile), 8);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdCreateDirectory), 9);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdRemoveDirectory), 10);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdOpenFileWO), 11);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdTruncateFile), 12);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdRename), 13);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdCalcFileCRC32), 14);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kCmdBurstReadFile), 15);
}

void MavlinkFTPTest::_testErrorCodeEnumValues()
{
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrNone), 0);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrFail), 1);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrFailErrno), 2);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrInvalidDataSize), 3);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrInvalidSession), 4);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrNoSessionsAvailable), 5);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrEOF), 6);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrUnknownCommand), 7);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrFailFileExists), 8);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrFailFileProtected), 9);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kErrFailFileNotFound), 10);
}

void MavlinkFTPTest::_testResponseOpCodeRange()
{
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kRspAck), 128);
    QCOMPARE_EQ(static_cast<int>(MavlinkFTP::kRspNak), 129);

    QVERIFY(static_cast<int>(MavlinkFTP::kRspAck) > static_cast<int>(MavlinkFTP::kCmdBurstReadFile));
}

// ============================================================================
// Structure Size Tests (Critical for Wire Protocol)
// ============================================================================

void MavlinkFTPTest::_testRequestHeaderSize()
{
    QCOMPARE_EQ(sizeof(MavlinkFTP::RequestHeader), 12u);
}

void MavlinkFTPTest::_testRequestSize()
{
    constexpr size_t payloadSize = sizeof(((mavlink_file_transfer_protocol_t*)0)->payload);

    // Request struct may have padding due to union alignment
    // Verify it fits within the mavlink payload with minimal overhead
    QVERIFY(sizeof(MavlinkFTP::Request) >= payloadSize);
    QVERIFY(sizeof(MavlinkFTP::Request) <= payloadSize + 4);  // Allow small alignment padding
}

void MavlinkFTPTest::_testRequestDataSize()
{
    constexpr size_t payloadSize = sizeof(((mavlink_file_transfer_protocol_t*)0)->payload);
    constexpr size_t headerSize = sizeof(MavlinkFTP::RequestHeader);
    constexpr size_t expectedDataSize = payloadSize - headerSize;

    MavlinkFTP::Request request;
    QCOMPARE_EQ(sizeof(request.data), expectedDataSize);

    QVERIFY(sizeof(request.data) > 0);
}
