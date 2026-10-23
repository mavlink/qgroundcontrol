#pragma once

#include "VehicleConfigUITestBase.h"

class Fact;
class MockLink;

/// UI tests for the PX4 Flight Modes setup page with a PX4 MockLink vehicle connected
class PX4FlightModesUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

private slots:
    void _overlappingSwitchesWarned();
    void _selectingButtonsClearsModeChannel();
    void _assignedFilterHidesUnassignedSwitches();
    void _releasedModeButtonKeepsSlot();

private:
    /// Navigates from the Fly view to the Flight Modes page of the Configure view
    void _navigateToFlightModes();

    /// Sets \a fact and waits for the vehicle to accept it
    void _setFact(Fact* fact, const QVariant& rawValue);

    /// Sends RC_CHANNELS with channel 7 at \a pwm and the other channels at rest
    static void _sendChannel7(MockLink* mockLink, uint16_t pwm);
};
