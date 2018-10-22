#ifndef AIRMAP_EVALUATION_H_
#define AIRMAP_EVALUATION_H_

#include <airmap/optional.h>
#include <airmap/ruleset.h>
#include <airmap/status.h>

#include <cstdint>
#include <iosfwd>
#include <iostream>
#include <string>
#include <vector>

namespace airmap {

/// Evaluation bundles together information regarding an airspace ruleset evaluation.
struct Evaluation {
  /// Authority models an authority capable of authorizing flight plans.
  struct Authority {
    std::string id;    ///< The id of the authority.
    std::string name;  ///< The name of the authority.
  };

  /// Authorization bundles up the authorization status of a flight plan.
  struct Authorization {
    /// Status enumerates all known states of an Authorization.
    enum class Status {
      accepted,                  ///< The flight plan is accepted.
      rejected,                  ///< The flight plan is rejected.
      pending,                   ///< The request for authorization is pending a response.
      accepted_upon_submission,  ///< The request will be accepted on submission.
      rejected_upon_submission   ///< The request will be rejected on submission.
    };

    Status status;        ///< The overall status of the request.
    Authority authority;  ///< The authority that handles the request.
    std::string message;  ///< The human-readable message provided by the authority.
  };

  /// Validation bundles up the validation status of a flight plan.
  struct Validation {
    /// Status enumerates all known states of a Validation.
    enum class Status {
      valid,    ///< The validation succeeded.
      invalid,  ///< The validation was rejected.
      unknown   ///< The status is unknown.
    };

    /// Feature describes a specific feature that requires validation.
    struct Feature {
      std::string code;         ///< The code of the feature.
      std::string description;  ///< The description of the feature.
    };

    Status status;        ///< The overall status of the validation.
    std::string data;     ///< The data provided for validation.
    std::string message;  ///< The human-readable message provided by the authority.
    Feature feature;      ///< The specific feature requiring validation.
    Authority authority;  ///< The authority carrying out the validation.
  };

  /// Failure enumrates all known failures during evaluation.
  enum class Failure {
    validation,     ///< The validation failed.
    authorization,  ///< The authorization failed.
    rulesets        ///< The ruleset engine failed.
  };

  std::vector<RuleSet> rulesets;        ///< All RuleSet instances relevant to a specific briefing/flight plan.
  std::vector<Validation> validations;  ///< All Validation instances relevant to a specific briefing/flight plan.
  std::vector<Authorization>
      authorizations;             ///< All Authorization instances relevant to a specific briefing/flight plan.
  std::vector<Failure> failures;  ///< All Failure instances relevant to a specific briefing/flight plan.
};

/// @cond
std::ostream& operator<<(std::ostream& out, Evaluation::Authorization::Status status);
std::istream& operator>>(std::istream& in, Evaluation::Authorization::Status& status);

std::ostream& operator<<(std::ostream& out, Evaluation::Validation::Status status);
std::istream& operator>>(std::istream& in, Evaluation::Validation::Status& status);

std::ostream& operator<<(std::ostream& out, Evaluation::Failure failure);
std::istream& operator>>(std::istream& in, Evaluation::Failure& failure);
/// @endcond

}  // namespace airmap

#endif  // AIRMAP_EVALUATION_H_
