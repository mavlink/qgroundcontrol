#pragma once

#include "UnitTest.h"

class NTRIPConnectionStatsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testInitialState();
    void testRecordMessage();
    void testReset();
    void testDataRate();
    void testCorrectionAgeInitial();
    void testCorrectionAgeAfterMessage_data();
    void testCorrectionAgeAfterMessage();
    void testInvalidReceiptTimestamp_data();
    void testInvalidReceiptTimestamp();
    void testMessageCountsByIdSortedAndReset();
    void testDataStaleAfterNoRecentMessages();
};
