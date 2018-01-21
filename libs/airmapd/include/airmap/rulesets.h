#ifndef AIRMAP_RULESETS_H_
#define AIRMAP_RULESETS_H_

#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/evaluation.h>
#include <airmap/geometry.h>
#include <airmap/outcome.h>
#include <airmap/ruleset.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace airmap {

/// RuleSets provides functionality for managing contextual airspace.
class RuleSets : DoNotCopyOrMove {
 public:
  /// Search bundles up types to ease interaction with
  /// RuleSets::search.
  struct Search {
    struct Parameters {
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
  struct ForId {
    struct Parameters {
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
  struct FetchRules {
    struct Parameters {
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
  struct EvaluateRules {
    struct Parameters {
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

 protected:
  /// @cond
  RuleSets() = default;
  /// @endcond
};

}  // namespace airmap

#endif  // AIRMAP_RULESETS_H_
