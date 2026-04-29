#pragma once

#include "UnitTest.h"

class SigningControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void cleanup();

    void _testInitialState();
    void _testBeginEnableTransitionsToEnabling();
    void _testBeginDisableTransitionsToDisabling();
    void _testReentryRejected();
    void _testEnableTimeoutFails();
    void _testDisableTimeoutVehicleUnreachable();
    void _testDestructorDuringPendingDoesNotCrash();
    void _testDestructorDuringEnableRestoresAutoDetect();
    void _testDestructorDuringDisableRestoresStrictCallback();
    void _testCancelDuringPendingEnable();
    void _testCancelDuringPendingDisable();
    void _testBadSignatureAlertThreshold();
    void _testBadSignatureAlertResetOnOk();
    void _testBadSignatureAlertEdgeTriggered();
    void _testResetForLinkClearsBurstState();
    void _testKeyAutoDetectedEmittedOnMatch();
    void _testKeyAutoDetectedNotEmittedWhenChannelSigning();
    void _testKeyAutoDetectedNotEmittedOnUnsigned();
    void _testStateOff();
    void _testStateOn();
    void _testStatusTextOff();
    void _testStatusTextOn();
    void _testStateChangedFiresOnEnableThenCancel();
    void _testStateChangedFiresOnDisableThenCancel();
};
