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

class SendMavCommandWithSignallingTest : public UnitTest
{
    Q_OBJECT
    
private slots:
    void _noFailure             (void);
    void _failureShowError      (void);
    void _failureNoShowError    (void);
    void _noFailureAfterRetry   (void);
    void _failureAfterRetry     (void);
    void _failureAfterNoReponse (void);
    void _unexpectedAck         (void);

private:
};
