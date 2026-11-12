#pragma once

#include "UnitTest.h"

/// The Saved Casters group of the RTK Corrections page: saving the caster that is entered, using and deleting a
/// saved one, and what it says when it cannot.
class NTRIPSavedCastersTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _savesAndUsesTheEnteredCaster();
    void _usingWaitsForADisconnect();
    void _saveSaysWhenItCannot();
};
