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
#ifndef AIRMAP_AIRCRAFTS_H_
#define AIRMAP_AIRCRAFTS_H_

#include <airmap/aircraft.h>
#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/optional.h>
#include <airmap/outcome.h>
#include <airmap/visibility.h>

#include <functional>
#include <string>
#include <vector>

namespace airmap {

/// Aircrafts models access to a database of aircraft models (specifically drones)
/// and manufacturers.
class AIRMAP_EXPORT Aircrafts : DoNotCopyOrMove {
 public:
  /// Manufacturers groups together types to ease interaction with
  /// Aircrafts::manufacturers.
  struct AIRMAP_EXPORT Manufacturers {
    /// Parameters bundles up input parameters.
    struct AIRMAP_EXPORT Parameters {
      Optional<std::string> manufacturer_name;  ///< Search for the specific manufacturer with this name.
    };

    /// Result models the outcome of calling Flights::manufacturers.
    using Result = Outcome<std::vector<Aircraft::Manufacturer>, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Flights::manufacturers finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// Models groups together types to ease interaction with
  /// Aircrafts::models.
  struct AIRMAP_EXPORT Models {
    /// Parameters bundles up input parameters.
    struct AIRMAP_EXPORT Parameters {
      Optional<Aircraft::Manufacturer> manufacturer;  ///< Only list models by this manufacturer.
      Optional<std::string> model_name;               ///< Search for the specific model with this name.
    };

    /// Result models the outcome of calling Flights::models.
    using Result = Outcome<std::vector<Aircraft>, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Flights::models finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// ModelForId groups together types to ease interaction with
  /// Aircrafts::model_for_id.
  struct AIRMAP_EXPORT ModelForId {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string id;  ///< Search for the model with this id.
    };

    /// Result models the outcome of calling Flights::model_for_id.
    using Result = Outcome<Aircraft, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Flights::model_for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// manufacturers queries the AirMap services for known aircraft
  /// manufacturers, reporting results to 'cb'.
  virtual void manufacturers(const Manufacturers::Parameters& parameters, const Manufacturers::Callback& cb) = 0;

  /// models queries the AirMap services for detailed information about
  /// known Aircraft models and reports back results to 'cb'.
  virtual void models(const Models::Parameters& parameters, const Models::Callback& cb) = 0;

  /// models queries the AirMap services for detailed information about
  /// an Aircraft model identified by 'ModelForId::Parameters::id' and reports back results to 'cb'.
  virtual void model_for_id(const ModelForId::Parameters& parameters, const ModelForId::Callback& cb) = 0;

 protected:
  /// @cond
  Aircrafts() = default;
  /// @endcond
};

}  // namespace airmap

#endif  // AIRMAP_AIRCRAFTS_H_
