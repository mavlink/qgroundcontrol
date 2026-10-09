#pragma once

#include "UnitTest.h"

class GPSReceiver;

class GPSReceiverConnectionPolicyTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _lostConnectionRetriesWithBackoff();
    void _connectConfiguredValidationErrors_data();
    void _connectConfiguredValidationErrors();
    void _configurationChangeCancelsRetry_data();
    void _configurationChangeCancelsRetry();
    void _sessionEndMessages_data();
    void _sessionEndMessages();
    void _pollingTicksReceiver();
#ifndef QGC_NO_SERIAL_LINK
    void _discoveryWaitsAndReconnects();
    void _savedReceiverReplacesDiscovery_data();
    void _savedReceiverReplacesDiscovery();
    void _excludedPorts_data();
    void _excludedPorts();
    void _retryBacksOffAndRespectsReservations();
    void _compositeReceiverSelection_data();
    void _compositeReceiverSelection();
    void _disconnectPausesAutomaticConnection();
    void _savedSerialWaitsForReturningPort_data();
    void _savedSerialWaitsForReturningPort();
    void _retryOfListedPortBacksOff();
    void _discoveryUsesSavedManufacturer_data();
    void _discoveryUsesSavedManufacturer();
#endif

private:
    static bool _retryPending(const GPSReceiver& receiver);
};
