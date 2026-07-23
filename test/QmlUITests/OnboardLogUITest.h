#pragma once

#include "QmlUITestBase.h"

/// UI smoke tests for the Analyze view Onboard Logs page, covering both the
/// message-based (LOG_*) and MAVLink FTP download transports.
class OnboardLogUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _messagesDownloadUITest();
    void _ftpDownloadUITest();
    void _messagesFullPageUITest();
    void _ftpFullPageUITest();

private:
    void _navigateToOnboardLogsPage();
    void _downloadFirstLogAndVerify(const QString& downloadDir);
    void _verifyButtonEnabled(const QString& objectName, bool expected);
    void _verifyIdleButtonStates(int logCount);
    void _verifySelectionButtonStates(bool hasSelection);
    void _fullPageStateChecks(int logCount);
    void _eraseAllAndVerifyEmpty();

    bool _ftpTransport = false; ///< Set by each test; drives Erase Selected visibility expectations
};
