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
#ifndef AIRMAP_AIRCRAFT_H_
#define AIRMAP_AIRCRAFT_H_

#include <airmap/visibility.h>

#include <string>

namespace airmap {

/// Aircraft describes an aircraft in terms of its model and its manufacturer.
struct AIRMAP_EXPORT Aircraft {
  /// Model bundles up a model id and a product name.
  struct AIRMAP_EXPORT Model {
    std::string id;    ///< The unique id of the model in the context of AirMap.
    std::string name;  ///< The human-readable name of the model.
  };

  /// Manufacturer bundles up an id and a human-readable name.
  /// Please note that the id is only unique/relevant in the context of the
  /// AirMap services.
  struct AIRMAP_EXPORT Manufacturer {
    std::string id;    ///< The unique id of the manufacturer in the context of AirMap.
    std::string name;  ///< The human-readable name of the manufacturer.
  };

  Model model;                ///< Details describing the model of an aircraft.
  Manufacturer manufacturer;  ///< Details about the manufacturer of an aircraft.
};

}  // namespace airmap

#endif  // AIRMAP_AIRCRAFT_H_
