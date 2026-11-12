#pragma once

#include "UnitTest.h"

/// The GNSS Receiver and RTK Corrections settings pages as the application shows them: over the application's GPS
/// services, which follow the saved settings as they do after startup. Each test takes one path from a field of the
/// page through its setting and its service to the status the page shows, against a peer on this machine.
///
/// The other GPS suites give a panel a service of their own, or the application's services without their settings.
/// Binding those to the settings, GPSManager::init(), cannot be undone, so this suite is registered standalone and
/// has a process to itself: run it through CTest.
class GPSSettingsPagesTest : public UnitTest
{
    Q_OBJECT

private slots:
    void initTestCase() override;
    void _receiverConnectsFromPage();
    void _ntripConnectsFromPage();
};
