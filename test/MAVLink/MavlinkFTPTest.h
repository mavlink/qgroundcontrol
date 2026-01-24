#pragma once

#include "TestFixtures.h"

/// Unit tests for MavlinkFTP protocol constants and string mappings.
/// Tests opcode/error code conversions and protocol structure validation.
class MavlinkFTPTest : public OfflineTest
{
    Q_OBJECT

public:
    MavlinkFTPTest() = default;

private slots:
    // OpCode string tests - individual opcodes
    void _testOpCodeNone();
    void _testOpCodeTerminateSession();
    void _testOpCodeResetSessions();
    void _testOpCodeListDirectory();
    void _testOpCodeOpenFileRO();
    void _testOpCodeReadFile();
    void _testOpCodeCreateFile();
    void _testOpCodeWriteFile();
    void _testOpCodeRemoveFile();
    void _testOpCodeCreateDirectory();
    void _testOpCodeRemoveDirectory();
    void _testOpCodeOpenFileWO();
    void _testOpCodeTruncateFile();
    void _testOpCodeRename();
    void _testOpCodeCalcFileCRC32();
    void _testOpCodeBurstReadFile();
    void _testOpCodeAck();
    void _testOpCodeNak();
    void _testOpCodeUnknown();

    // ErrorCode string tests - individual error codes
    void _testErrorCodeNone();
    void _testErrorCodeFail();
    void _testErrorCodeFailErrno();
    void _testErrorCodeInvalidDataSize();
    void _testErrorCodeInvalidSession();
    void _testErrorCodeNoSessionsAvailable();
    void _testErrorCodeEOF();
    void _testErrorCodeUnknownCommand();
    void _testErrorCodeFileExists();
    void _testErrorCodeFileProtected();
    void _testErrorCodeFileNotFound();
    void _testErrorCodeUnknown();

    // Protocol constant tests
    void _testOpCodeEnumValues();
    void _testErrorCodeEnumValues();
    void _testResponseOpCodeRange();

    // Structure size tests (critical for wire protocol)
    void _testRequestHeaderSize();
    void _testRequestSize();
    void _testRequestDataSize();
};
