#pragma once

#include "UnitTest.h"

/// Unit tests for ADSBTCPLink class.
/// These tests require network (localhost TCP) and are excluded from CI.
class ADSBTCPLinkTest : public UnitTest
{
    Q_OBJECT

public:
    ADSBTCPLinkTest() = default;

private slots:
    void _connectionTest();
    void _messageParsingTest();
    void _errorHandlingTest();
};
