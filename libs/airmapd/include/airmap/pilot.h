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
#ifndef AIRMAP_PILOT_H_
#define AIRMAP_PILOT_H_

#include <airmap/aircraft.h>
#include <airmap/date_time.h>
#include <airmap/optional.h>
#include <airmap/visibility.h>

#include <cstdint>

#include <map>
#include <string>

namespace airmap {

/// Pilot bundles up all properties describing a pilot on the AirMap services.
struct AIRMAP_EXPORT Pilot {
  /// Aircraft describes a vehicle owned by a Pilot.
  struct AIRMAP_EXPORT Aircraft {
    std::string id;          ///< The unique id of the vehicle in the context of AirMap.
    std::string nick_name;   ///< The human-readable nickname of the vehicle.
    airmap::Aircraft model;  ///< The model of the aircraft.
    DateTime created_at;     ///< Timestamp marking the creation of the device in the AirMap system.
  };

  std::string id;                     ///< The unique id of the pilot in the context of AirMap.
  std::string first_name;             ///< The first name of the pilot.
  std::string last_name;              ///< The last name of the pilot.
  std::string email;                  ///< The email AirMap recorded for this pilot.
  Optional<std::string> user_name;    ///< The pilot's username used to connect to AirMap services.
  Optional<std::string> picture_url;  ///< The URL of a picture showing the pilot.

  /// VerificationStatus summarizes the
  /// status of contact detail verification.
  struct AIRMAP_EXPORT VerificationStatus {
    bool email;  ///< true iff the email address of the pilot has been verified
    bool phone;  ///< true iff the phone number of the pilot has been verified
  } verification_status;

  /// Statistics about the pilot and her
  /// flight experience as recorded by the
  /// AirMap services.
  struct AIRMAP_EXPORT Statistics {
    struct AIRMAP_EXPORT Flight {
      std::uint64_t total;        ///< The total number of flights
      DateTime last_flight_time;  ///< Date and time of the last flight
    } flight;                     ///< Statistical details about flights conducted by a pilot.
    struct AIRMAP_EXPORT Aircraft {
      std::uint64_t total;  ///< The total number of aircrafts
    } aircraft;             ///< Statistical details about aircrafts owned by a pilot
  } statistics;

  /// App- and user-specific metadata.
  struct AIRMAP_EXPORT Metadata {
    struct AIRMAP_EXPORT App {
      struct AIRMAP_EXPORT Legal {
        struct AIRMAP_EXPORT PrivacyPolicy {
          bool update_required;   ///< True iff user needs to accept updated privacy policy
        } privacy_policy;
        struct AIRMAP_EXPORT TermsOfService {
          bool update_required;   ///< True iff user needs to accept updated terms of service
        } terms_of_service;
      } legal;
      std::uint64_t total;        ///< The total number of flights
      DateTime last_flight_time;  ///< Date and time of the last flight
    } app;                        ///< App-specific metadata
    std::map<std::string, std::string> user;  ///< User-specific meta-data.
  } metadata;                                 ///< Metadata associated with a pilot.
  DateTime created_at;                        ///< Timestamp of the creation of this pilot in the AirMap system.
};

}  // namespace airmap

#endif  // AIRMAP_PILOT_H_
