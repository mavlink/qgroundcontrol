#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class VehicleFileFetchTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    UT_PARAMETERIZED_TEST(_testFetch);
    UT_PARAMETERIZED_TEST(_testFailedFetchLeavesNothing);
    UT_PARAMETERIZED_TEST(_testFileOverTheLimit);
    UT_PARAMETERIZED_TEST(_testFetchEndedWithoutCallback);
    void _testFetchesOfOneVehicleQueue();
    UT_PARAMETERIZED_TEST(_testRefused);
    UT_PARAMETERIZED_TEST(_testUriOtherThanFtpOrHttpRefused);
    UT_PARAMETERIZED_TEST(_testHttpUriForms);
    void _testHttpCache();
    UT_PARAMETERIZED_TEST(_testServerCertificate);

private:
    /// What a fetch called back with
    struct Result
    {
        int calls = 0;
        QString file;
        QString error;
    };
};
