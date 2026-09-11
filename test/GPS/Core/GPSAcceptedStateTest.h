#pragma once

#include "UnitTest.h"

class GPSAcceptedStateTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _relativeExpiryAndSession();
    void _integrityProvenanceExpiry();
    void _integrityRejectsOlderGroups();
    void _schedulerDestructionClearsAcceptedState();
};
