#pragma once

#include "BaseClasses/VehicleTest.h"
#include "BaseClasses/VehicleTestManualConnect.h"

/// Tests the message-based (LOG_REQUEST_LIST/LOG_REQUEST_DATA) transport of OnboardLogController.
/// MockLink does not advertise MAV_PROTOCOL_CAPABILITY_FTP by default so the controller
/// must select the message transport.
class OnboardLogDownloadTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _downloadTest();
    void _selectAllTest();
    void _cancelDownloadTest();
    void _vehicleDisconnectDuringDownloadTest();
    void _eraseAllTest();
};

/// Tests the MAVLink FTP transport of OnboardLogController against a MockLink which
/// advertises MAV_PROTOCOL_CAPABILITY_FTP and serves logs from the @MAV_LOG virtual directory.
class OnboardLogFtpDownloadTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _ftpListAndDownloadTest();
    void _ftpListFallbackTest();
    void _ftpMultiDownloadAndDedupTest();
    void _ftpDownloadErrorDisablesFtpTest();
    void _ftpSortOrderTest();
    void _ftpCancelDownloadTest();
    void _ftpEraseSelectedTest();
    void _ftpCancelEraseSelectedTest();
};
