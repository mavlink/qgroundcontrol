// added by NJD needs to be made conformal to standards
// first whack kind of BS
// based on VehicleMETFactGroup.h

/*
 * What follows is enabled by uncommenting the lines of code marked with "findMe" comments in Vehicle.cc and Vehicle.h
 * The feature enabled by that is unstable and causes a memory access error within Qt after replaying a log for a bit
 * I don't know why.
 *
 * Explanation of this system:
 * VehicleMETFactGroup is child of the FactGroup class that is responsible for defining what this fact is
 * A single instance of this is owned by vehicle in vehicle.h (along with a FactGroupName in both the header and source)
 * The constructor is called in Vehicle::_commonInit()
 * The constructor register itself as a fact group, and adds each fact to itself
 * (presumably, not verified) there is a background engine that updates the fact group based on the interval specified in the constructor
 *
 * The facts themselves only get updated when handleMessage gets called on each message (from approx 798 in vehicle.cc)
 * handleMessage must do nothing for messages it doesn't care about, and do something with the ones it does, including decode them
 * The information must then get saved to a fact by calling fat::setRawValue().
 *
 * The architecture shown here does NOT do anything to filter or order information.
 * The algorithms for detecting an ascent, binning, etc must occur elsewhere.
 * Note that parsing algorithms must be safe. I use reinterpret_cast in handleMessage without error handling - bad! Right?
 * If for some reason it were to fail to convert the binary data to a float, then it would crash with a run time error!
 * The extent to which we can trust the incoming data to be good is unknown, so only time will tell just how safe we need to be.
 *
 * Lastly, I have included a reference to AHRS2 in handleMessage. This already has a message hanlder somewhere. We could rehandle it.
 * Rehandling may be the best way to go, depending on where the processing algorithms are placed. Mostly likely not, though. It's
 * just an example and may not be relevant.
*/

#pragma once

#include "FactGroup.h"

class Vehicle;

class VehicleMETFactGroup : public FactGroup
{
    Q_OBJECT

public:
    VehicleMETFactGroup(QObject* parent = nullptr);

    Q_PROPERTY(Fact* temperature              READ temperature              CONSTANT)
    Q_PROPERTY(Fact* rhum                     READ rhum                     CONSTANT)
    //Q_PROPERTY(Fact* tempR              READ tempR              CONSTANT) // place holder becauce we don't actually care about this

    Fact* temperature                         () { return &_temperatureFact; }
    Fact* rhum                                () { return &_temperatureFact; }

    // Overrides from FactGroup
    void handleMessage(Vehicle* vehicle, mavlink_message_t& message) override;

    static const char* _temperatureFactName;
    static const char* _rhumFactName;

private:
    struct _content
    {
        float temperature1 = 0;
        float temperature2 = 0;
        float temperature3 = 0;
        float rhum1 = 0;
        float rhum2 = 0;
        float rhum3 = 0;
        float tempR1 = 0;
        float tempR2 = 0;
        float tempR3 = 0;
        float pressure1 = 0;
        float windSpeed = 0;
        float windDirection = 0;
        // etc, fill in everything we need to know
    };
private:
    _content _currentContents;

    // one entry here per fact
    Fact _temperatureFact;
    Fact _rhumFact;

};
