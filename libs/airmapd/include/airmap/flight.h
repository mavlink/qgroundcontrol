#ifndef AIRMAP_FLIGHT_H_
#define AIRMAP_FLIGHT_H_

#include <airmap/date_time.h>
#include <airmap/flight_plan.h>
#include <airmap/geometry.h>
#include <airmap/pilot.h>

#include <cstdint>

namespace airmap {

/// Flight bundles together properties describing an individual flight.
struct Flight {
  using Id = std::string;

  Id id;                                    ///< The unique identifier of a flight in the context of AirMap.
  Optional<FlightPlan::Id> flight_plan_id;  ///< The flight plan corresponding to this flight.
  Pilot pilot;                              ///< The pilot responsible for the flight.
  Pilot::Aircraft aircraft;                 ///< The aircraft conducting the flight.
  float latitude;                           ///< The latitude component of the takeoff point in [°].
  float longitude;                          ///< The longitude component of the takeoff point in [°].
  float max_altitude;                       ///< The maximum altitude over the entire flight in [m].
  Geometry geometry;                        ///< The geometry describing the flight.
  DateTime created_at;                      ///< Point in time when the flight was created.
  DateTime start_time;                      ///< Point in time when the flight will start/was started.
  DateTime end_time;                        ///< Point in time when the fligth will end.
};

}  // namespace airmap

#endif  // AIRMAP_FLIGHT_H_
