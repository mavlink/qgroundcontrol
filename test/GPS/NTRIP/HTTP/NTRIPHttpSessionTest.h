#pragma once

#include "UnitTest.h"

class NTRIPHttpSessionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _responseEndsWithFinished();
    void _invalidConfigDoesNotConnect();
    void _refusedConnectionFailsOnce();
    void _abortIsSilent();
    void _retireFromDeliveryDetachesOwner();
};
