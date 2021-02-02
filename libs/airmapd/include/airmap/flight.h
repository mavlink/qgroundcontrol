// AirMap Platform SDK
// Copyright © 2018 AirMap, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef AIRMAP_FLIGHT_H_
#define AIRMAP_FLIGHT_H_

#include <airmap/date_time.h>
#include <airmap/flight_plan.h>
#include <airmap/geometry.h>
#include <airmap/pilot.h>
#include <airmap/visibility.h>

#include <cstdint>

namespace airmap {

/// Flight bundles together properties describing an individual flight.
struct AIRMAP_EXPORT Flight {
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
