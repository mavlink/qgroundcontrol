#pragma once

#include "UnitTest.h"

class VehicleGPSAggregateFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _independentExpiry_data();
    void _independentExpiry();
    void _freshAuthenticationPrecedence_data();
    void _freshAuthenticationPrecedence();
    void _bindPreservesReceiptAge();
    void _rebindDisconnectsPreviousReceivers();
    void _onlyIntegrityRefreshesReceipt();
    void _receiverDestruction();
    void _schedulerDestruction();
    void _reentrantRebind();
};
