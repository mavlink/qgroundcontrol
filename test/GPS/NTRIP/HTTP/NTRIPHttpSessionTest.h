#pragma once

#include "UnitTest.h"

class NTRIPHttpSessionTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _responseEndsWithFinished();
    void _invalidConfigDoesNotConnect();
    void _refusedConnectionFailsOnce();
    void _unansweredRequestIsRepeatedInVersion1_data();
    void _unansweredRequestIsRepeatedInVersion1();
    void _requestIsRepeatedOnlyOnce();
    void _answeredRequestIsNotRepeated();
    void _casterVersionsKeepTheLatestCasters();
    void _casterVersionsAreSavedAndRestored_data();
    void _casterVersionsAreSavedAndRestored();
    void _version1MemoryFollowsTheCaster_data();
    void _version1MemoryFollowsTheCaster();
    void _version1MemoryOutlastsLocalFailures();
    void _acceptedVersion2RequestForgetsVersion1();
    void _abortIsSilent();
    void _retireFromDeliveryDetachesOwner();
};
