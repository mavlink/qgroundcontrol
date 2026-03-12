#pragma once

#include "UnitTest.h"

class SettingsLifecycleTest : public UnitTest
{
    Q_OBJECT

private slots:
    void testEnableDisableCycle();
    void testSettingsChangeRestartsConnection();
    void testErrorThenReconnectCycle();
    void testRapidToggleDoesNotCrash();
    void testDisableDuringReconnect();
};
