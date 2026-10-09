#pragma once

#include "UnitTest.h"

class NTRIPConnectionStatsTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _noFirstCorrectionBecomesStale();
    void _reset();
    void _dataRate();
    void _correctionAgeAfterMessage_data();
    void _correctionAgeAfterMessage();
    void _invalidReceiptTimestamp_data();
    void _invalidReceiptTimestamp();
    void _messageCountsByIdSortedAndReset();
    void _dataStaleAfterNoRecentMessages();
    void _statisticsExpireDuringSilence();
};
