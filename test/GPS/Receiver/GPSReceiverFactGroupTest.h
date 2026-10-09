#pragma once

#include "UnitTest.h"

/// The receiver's status Facts, written as the receiver reports.
class GPSReceiverFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _defaultsWithoutReceiver();
    void _satelliteCounts_data();
    void _satelliteCounts();
    void _currentBaseSaveValidity_data();
    void _currentBaseSaveValidity();
    void _integrityFacts();
    void _silentReceiverClearsSolution();
    void _summaryLabel();
};
