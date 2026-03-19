#pragma once

#include "BaseClasses/VehicleTest.h"

class MockLinkSigningTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit MockLinkSigningTest(QObject* parent = nullptr);

private slots:
    void init() override;
    void cleanup() override;
    void _testSendSetupSigning();
    void _testSendDisableSigning();
    void _testSigningKeysAddRemove();
};
