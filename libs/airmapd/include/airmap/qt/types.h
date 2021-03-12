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
#ifndef AIRMAP_QT_TYPES_H_
#define AIRMAP_QT_TYPES_H_

#include <airmap/aircraft.h>
#include <airmap/airspace.h>
#include <airmap/credentials.h>
#include <airmap/date_time.h>
#include <airmap/flight.h>
#include <airmap/flight_plan.h>
#include <airmap/geometry.h>
#include <airmap/optional.h>
#include <airmap/outcome.h>
#include <airmap/pilot.h>
#include <airmap/rule.h>
#include <airmap/ruleset.h>
#include <airmap/status.h>
#include <airmap/telemetry.h>
#include <airmap/token.h>
#include <airmap/traffic.h>
#include <airmap/version.h>
#include <airmap/visibility.h>

#include <QMetaType>

Q_DECLARE_METATYPE(airmap::Aircraft)
Q_DECLARE_METATYPE(airmap::Airspace)
Q_DECLARE_METATYPE(airmap::Credentials)
Q_DECLARE_METATYPE(airmap::DateTime)
Q_DECLARE_METATYPE(airmap::Error)
Q_DECLARE_METATYPE(airmap::FlightPlan)
Q_DECLARE_METATYPE(airmap::Flight)
Q_DECLARE_METATYPE(airmap::Geometry)
Q_DECLARE_METATYPE(airmap::Pilot)
Q_DECLARE_METATYPE(airmap::Rule)
Q_DECLARE_METATYPE(airmap::RuleSet)
Q_DECLARE_METATYPE(airmap::RuleSet::Rule)
Q_DECLARE_METATYPE(airmap::Status::Advisory)
Q_DECLARE_METATYPE(airmap::Status::Wind)
Q_DECLARE_METATYPE(airmap::Status::Weather)
Q_DECLARE_METATYPE(airmap::Status::Report)
Q_DECLARE_METATYPE(airmap::Telemetry::Position)
Q_DECLARE_METATYPE(airmap::Telemetry::Speed)
Q_DECLARE_METATYPE(airmap::Telemetry::Attitude)
Q_DECLARE_METATYPE(airmap::Telemetry::Barometer)
Q_DECLARE_METATYPE(airmap::Optional<airmap::Telemetry::Update>)
Q_DECLARE_METATYPE(airmap::Token::Type)
Q_DECLARE_METATYPE(airmap::Token::Anonymous)
Q_DECLARE_METATYPE(airmap::Token::OAuth)
Q_DECLARE_METATYPE(airmap::Token::Refreshed)
Q_DECLARE_METATYPE(airmap::Token)
Q_DECLARE_METATYPE(airmap::Traffic::Update::Type)
Q_DECLARE_METATYPE(airmap::Traffic::Update)
Q_DECLARE_METATYPE(airmap::Version)

namespace airmap {
namespace qt {

/// register_types makes airmap::* types known to the Qt type system.
///
/// This function has to be called at least once to be able to use airmap::*
/// types in queued signal-slot connections.
AIRMAP_EXPORT void register_types();

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_TYPES_H_
