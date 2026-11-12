#pragma once

#include "UnitTest.h"

/// The list of Android's serial ports and what the app asks about a port, built for the host. The USB ports
/// are those of a simulated Java side (AndroidSerialFake), the device nodes those of a directory of the
/// test's.
class AndroidSerialPortInfoTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void _posixPortsAreDescribedBySysfs();
    void _posixSettingAddsToTheUsbPorts();
    void _usbPermissionIsRequestedForAUsbPortThatNeedsIt();
    void _portInfoIsFoundByTheStableNameOfAPort();
};
