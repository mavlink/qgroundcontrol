#pragma once

#include "UnitTest.h"

class MAVLinkFTPTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testDirectoryEntryCodec();
    void _testMalformedDirectoryEntries();
    void _testMalformedDirectoryPayload_data();
    void _testMalformedDirectoryPayload();
    void _testRequestAndResponseCodec();
    void _testUriParsing();
    void _testUriValidation_data();
    void _testUriValidation();
    void _testGeneratedEnumStrings();
};
