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
#ifndef AIRMAP_RULESETS_H_
#define AIRMAP_RULESETS_H_

#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/evaluation.h>
#include <airmap/flight_plan.h>
#include <airmap/geometry.h>
#include <airmap/outcome.h>
#include <airmap/ruleset.h>
#include <airmap/visibility.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace airmap {

/// RuleSets provides functionality for managing contextual airspace.
class AIRMAP_EXPORT RuleSets : DoNotCopyOrMove {
 public:
  /// Search bundles up types to ease interaction with
  /// RuleSets::search.
  struct AIRMAP_EXPORT Search {
    struct AIRMAP_EXPORT Parameters {
      Required<Geometry> geometry;  ///< Search for rulesets intersecting this geometry.
    };

    /// Result models the outcome of calling RuleSets::search.
    using Result = Outcome<std::vector<RuleSet>, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to RuleSets::search finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// ForId bundles up types to ease interaction with
  /// RuleSets::for_id.
  struct AIRMAP_EXPORT ForId {
    struct AIRMAP_EXPORT Parameters {
      RuleSet::Id id;  ///< Search for the ruleset with this id.
    };

    /// Result models the outcome of calling RuleSets::for_id.
    using Result = Outcome<RuleSet, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to RuleSets::for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// FetchRules bundles up types to ease interaction with
  /// RuleSets::fetch_rules.
  struct AIRMAP_EXPORT FetchRules {
    struct AIRMAP_EXPORT Parameters {
      Optional<std::string> rulesets;  ///< Fetch rules which apply to these rulesets.
    };

    /// Result models the outcome of calling RuleSets::fetch_rules.
    using Result = Outcome<std::vector<RuleSet>, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to RuleSets::fetch_rules finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// EvaluateRules bundles up types to ease interaction with
  /// RuleSets::evaluate_rulesets.
  struct AIRMAP_EXPORT EvaluateRules {
    struct AIRMAP_EXPORT Parameters {
      Required<Geometry> geometry;  ///< Evaluate rulesets intersecting this geometry.
      std::unordered_map<std::string, RuleSet::Feature::Value>
          features;                    ///< Additional properties of the planned flight.
      Required<std::string> rulesets;  ///< Evaluate these rulesets.
    };

    /// Result models the outcome of calling RuleSets::evaluate_rulesets.
    using Result = Outcome<Evaluation, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to RuleSets::evaluate_rulesets finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// EvaluateFlightPlan bundles up types to ease interaction with
  /// RuleSets::evaluate_flight_plan.
  struct AIRMAP_EXPORT EvaluateFlightPlan {
    struct AIRMAP_EXPORT Parameters {
      FlightPlan::Id id;  ///< Id of the flight plan that should be submitted.
    };

    /// Result models the outcome of calling RuleSets::evaluate_flight_plan.
    using Result = Outcome<Evaluation, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to RuleSets::evaluate_rulesets finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// search queries the AirMap services for detailed information about
  /// rulesets identified by geometry and reports back results to 'cb'.
  virtual void search(const Search::Parameters& parameters, const Search::Callback& cb) = 0;

  /// for_id queries the AirMap services for detailed information about
  /// a ruleset identified by a UUID and reports back results to 'cb'.
  virtual void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) = 0;

  /// fetch_rules fetches rules from the rulesets identified by 'parameters' and
  /// reports back results to 'cb'.
  virtual void fetch_rules(const FetchRules::Parameters& parameters, const FetchRules::Callback& cb) = 0;

  /// evaluate_rulesets evaluates rulesets and geometry identified by 'parameters' and
  /// reports back results to 'cb'.
  virtual void evaluate_rulesets(const EvaluateRules::Parameters& parameters, const EvaluateRules::Callback& cb) = 0;

  /// evaluate_flight_plan evaluates a flight plan identified by 'parameters' and
  /// reports back results to 'cb'.
  virtual void evaluate_flight_plan(const EvaluateFlightPlan::Parameters& parameters, const EvaluateFlightPlan::Callback& cb) = 0;

 protected:
  /// @cond
  RuleSets() = default;
  /// @endcond
};

}  // namespace airmap

#endif  // AIRMAP_RULESETS_H_
