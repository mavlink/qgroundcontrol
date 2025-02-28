/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "UnitTest.h"
#include "MockConfiguration.h"

class ParameterManagerTest : public UnitTest
{
    Q_OBJECT
    
private slots:
    void _noFailure(void);
    void _requestListNoResponse(void);
    void _requestListMissingParamSuccess(void);
    void _requestListMissingParamFail(void);
    void _FTPnoFailure(void);
    // void _FTPChangeParam(void);


private:
    void _noFailureWorker(MockConfiguration::FailureMode_t failureMode);
};
