#pragma once

#include "UnitTest.h"

/// Android's QSerialPort over its USB backend, built for the host and run against a simulated Java side
/// (AndroidSerialFake): opening, transfers, buffers and settings while the device works.
class AndroidSerialPortUsbTest : public UnitTest
{
    Q_OBJECT

private slots:
    void init() override;
    void _portNameChoosesTheBackend();
    void _closedPortReportsNotOpen();
    void _settingReachesTheDevice_data();
    void _settingReachesTheDevice();
    void _controlLineSettersTellAChangeWithoutAskingTheDevice_data();
    void _controlLineSettersTellAChangeWithoutAskingTheDevice();
    void _controlLinesAreReadFromTheDevice();
    void _writesAreBufferedAndReported();
    void _outputReachesTheDeviceWholeAndInOrder();
    void _writeWaitTimeoutKeepsPendingBytes();
    void _closeSendsPendingWrites();
    void _writeOfNothingIsNoError();
    void _closeGivesUpStalledWritesQuietly();
    void _readWaitKeepsWriting();
    void _readWaitEndsOnNewInputOnly();
    void _readWaitInsideReadyReadDoesNotAnnounceAgain();
    void _fullReadBufferHoldsInputBack();
    void _partlyReadBufferTakesMoreInput();
    void _fullReadBufferPausesJavasReading();
    void _pausedReadingResumesWithoutARead_data();
    void _pausedReadingResumesWithoutARead();
    void _inputFromJavasThreadArrivesWholeThroughAFullBuffer_data();
    void _inputFromJavasThreadArrivesWholeThroughAFullBuffer();
    void _traceShowsWhatCrossesThePort();
    void _negativeReadBufferSizeIsNoLimit();
    void _linesAreFoundInTheInputThatThePortHolds();
    void _peekedInputStaysInputOfThePort();
    void _clearOutputGivesUpWhatTheDeviceHasNotTaken();
    void _outputAfterClearIsReportedBehindTheDiscardedBytes();
    void _incomingDataIsDeliveredOnOwnerThread();
    void _usbPortsHearOnlyWhatIsForThem();
    void _unregisteringAPortWaitsForAReportInProgress();
    void _unchangedParametersAreNotSentAgain();
    void _readThreadIsTrackedWithoutAskingTheBackend();
    void _clearInputDiscardsPartlyDeliveredData();
    void _clearInputPurgesTheDeviceBeforeReadingOn();
    void _clearDiscardsWhatTheDeviceHoldsInTheDirectionsGiven_data();
    void _clearDiscardsWhatTheDeviceHoldsInTheDirectionsGiven();
};
