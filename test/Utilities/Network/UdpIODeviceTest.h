#pragma once

#include "UnitTest.h"

class UdpIODeviceTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _receiptTimestampsSurviveBuffering();
    void _selectedPeerIsolation();
    void _peerReplacementClearsBuffers_data();
    void _peerReplacementClearsBuffers();
    void _peerReplacementCanRetireDevice_data();
    void _peerReplacementCanRetireDevice();
    void _readOnlyBinding();
    void _boundedDrainPublishesAllData();
    void _readyReadCanRetireDevice_data();
    void _readyReadCanRetireDevice();
    void _byteAccountingAndPeek();
    void _peekedFragmentBeforeNewline();
    void _skipBufferedData_data();
    void _skipBufferedData();
    void _fragmentedAndPartialLines();
    void _nmeaStartDiscardsBufferedData();
    void _overflowKeepsNewestLines();
    void _overflowDiscardsPartialLine();
    void _overflowAfterBufferedRead_data();
    void _overflowAfterBufferedRead();
    void _repeatedPeekRemainsBounded();
    void _transactionAcrossDatagrams();
    void _textTransactionAcrossDatagrams();
    void _textOverflowPreservesLines();
    void _closeClearsBufferedData();
};
