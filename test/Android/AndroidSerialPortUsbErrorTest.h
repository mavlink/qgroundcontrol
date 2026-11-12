#pragma once

#include "UnitTest.h"

/// Android's QSerialPort over its USB backend, built for the host and run against a simulated Java side
/// (AndroidSerialFake): what the port reports when a call fails or its device is lost.
class AndroidSerialPortUsbErrorTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void _openFailureReportsItsCause_data();
    void _openFailureReportsItsCause();
    void _failedOpenAddsWhatJavaSaysOfIt();
    void _openWithoutADescriptorClosesTheDevice();
    void _readWaitEndsWhenAWriteFails();
    void _writeFailureReportsItsKind_data();
    void _writeFailureReportsItsKind();
    void _ownerMayCloseFromTheErrorOfAFailedWrite();
    void _parametersAreSentAgainAfterAFailedSet();
    void _rejectedParametersReportOneError();
    void _failedCommandAddsWhatJavaSaysOfIt();
    void _commandOnALostDeviceReportsTheLossOnce_data();
    void _commandOnALostDeviceReportsTheLossOnce();
    void _writeOnALostDeviceReportsTheLossOnce();
    void _readOnALostDeviceReportsTheLossOnce();
    void _ownerMayCloseFromTheErrorOfACommandOnALostDevice();
    void _failedCloseOfALostDeviceIsQuiet_data();
    void _failedCloseOfALostDeviceIsQuiet();
    void _failedCloseReportsItsCause();
    void _refusedWriteAddsWhatJavaSaysOfIt();
    void _resourceErrorLeavesClosingToTheOwner();
    void _resourceErrorTextTellsTheReason_data();
    void _resourceErrorTextTellsTheReason();
    void _resourceErrorOfAClosedPortIsDropped();
    void _lostDeviceEndsABlockingWait_data();
    void _lostDeviceEndsABlockingWait();
    void _closeOfALostDeviceDoesNotWaitForItsOutput();
    void _readingOnAfterTheDeviceIsGoneIsQuiet_data();
    void _readingOnAfterTheDeviceIsGoneIsQuiet();
    void _unsupportedControlLineClassification_data();
    void _unsupportedControlLineClassification();
};
