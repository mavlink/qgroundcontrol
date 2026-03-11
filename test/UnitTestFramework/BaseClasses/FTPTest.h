#pragma once

#include "VehicleTest.h"

class FTPManager;

/// @file
/// @brief Base class for MAVLink FTP tests

/// Test fixture for MAVLink FTP (file transfer) tests.
///
/// Example usage:
/// @code
/// class MyFTPTest : public FTPTest
/// {
///     Q_OBJECT
/// private slots:
///     void _testFileList() {
///         ftpManager()->listDirectory("/");
///         // ...
///     }
/// };
/// @endcode
class FTPTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit FTPTest(QObject* parent = nullptr);
    ~FTPTest() override = default;

    /// Returns the FTP manager
    FTPManager* ftpManager() const;

protected slots:
    void init() override;

protected:
    /// Waits for FTP operation to complete
    bool waitForFTPComplete(int timeoutMs = 0);
};
