#ifndef AIRMAP_RULESET_H_
#define AIRMAP_RULESET_H_

#include <airmap/date_time.h>
#include <airmap/geometry.h>
#include <airmap/optional.h>
#include <airmap/pilot.h>
#include <airmap/status.h>

#include <cstdint>
#include <iosfwd>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace airmap {

/// RuleSet bundles together properties describing a ruleset.
struct RuleSet {
  // Feature describes a flight feature modelled by a particular rule.
  struct Feature;
  /// Rule models the individual result of a Rule evaluation.
  struct Rule {
    /// Status enumerates all known status codes of a rule.
    enum class Status {
      unknown,          ///< The status of the rule is unknown.
      conflicting,      ///< The rule is conflicting.
      not_conflicting,  ///< The rule is not conflicting, all good to go.
      missing_info,     ///< The evaluation requires further information.
      informational     ///< The rule is of informational nature.
    };

    Status status;                  ///< The status of the rule.
    std::string short_text;         ///< The human-readable short summary of the rule.
    std::string description;        ///< The human-readable description of the rule.
    std::int32_t display_order;     ///< An indicator for ordering the ruleset.
    std::vector<Feature> features;  ///< The features modelled by the rule.
  };

  /// SelectionType enumerates all known types for a RuleSet.
  enum class SelectionType {
    pickone,   ///< One rule from the overall set needs to be picked.
    required,  ///< Satisfying the RuleSet is required.
    optional   ///< Satisfying the RuleSet is not required.
  };

  /// Jurisdiction describes a jurisdiction in a geographical scope.
  struct Jurisdiction {
    /// Region enumerates all known regional scopes of a jurisdiction.
    enum class Region {
      national,  ///< The jurisdiction applies nation-wide.
      state,     ///< The jurisdiction applies to a specific state.
      county,    ///< The jurisdiction applies to a specific county.
      city,      ///< The jurisdiction applies to a specific city.
      local      ///< The jurisdiction only applies locally.
    };
    /// Id models a unique identifier for a jurisdiction in the context of AirMap.
    using Id = std::uint64_t;

    Id id;             ///< The unique id.
    std::string name;  ///< The human-readable name.
    Region region;     ///< The regional scope.
  };

  /// Id models a unique identifier for a briefing in the context of AirMap.
  using Id = std::string;

  Id id;                         ///< The unique id.
  SelectionType selection_type;  ///< The selection type.
  std::string name;              ///< The human-readable name.
  std::string short_name;        ///< The human-readable short name.
  std::string description;       ///< The human readable description.
  bool is_default;
  Jurisdiction jurisdiction;                ///< The jurisdiction.
  std::vector<std::string> airspace_types;  ///< The layers that a RuleSet instance applies to.
  std::vector<Rule> rules;                  ///< The individual rules in the set.

  struct Feature {
    enum class Type { unknown, boolean, floating_point, string };
    enum class Measurement { unknown, speed, weight, distance };
    enum class Unit { unknown, kilograms, meters, meters_per_sec };

    class Value {
     public:
      Value();
      explicit Value(bool value);
      explicit Value(double value);
      explicit Value(const std::string& value);
      Value(const Value& other);
      Value(Value&& other);
      ~Value();
      Value& operator=(const Value& other);
      Value& operator=(Value&& other);

      Type type() const;
      bool boolean() const;
      double floating_point() const;
      const std::string& string() const;

     private:
      Value& construct(const Value& other);
      Value& construct(Value&& other);
      Value& construct(bool value);
      Value& construct(double value);
      Value& construct(const std::string& value);
      Value& destruct();

      Type type_;
      union Detail {
        Detail();
        ~Detail();

        bool b;
        double d;
        std::string s;
      } detail_;
    };

    Optional<Value> value(bool b) const;
    Optional<Value> value(double d) const;
    Optional<Value> value(const std::string& s) const;

    std::int32_t id{-1};
    std::string name;
    Optional<std::string> code;
    std::string description;
    RuleSet::Rule::Status status;
    Type type{Type::unknown};
    Measurement measurement{Measurement::unknown};
    Unit unit{Unit::unknown};
  };
};

std::ostream& operator<<(std::ostream& out, RuleSet::Feature::Type type);
std::istream& operator>>(std::istream& in, RuleSet::Feature::Type& type);

std::ostream& operator<<(std::ostream& out, RuleSet::Feature::Measurement measurement);
std::istream& operator>>(std::istream& in, RuleSet::Feature::Measurement& measurement);

std::ostream& operator<<(std::ostream& out, RuleSet::Feature::Unit unit);
std::istream& operator>>(std::istream& in, RuleSet::Feature::Unit& unit);

std::ostream& operator<<(std::ostream& out, RuleSet::Jurisdiction::Region region);
std::istream& operator>>(std::istream& in, RuleSet::Jurisdiction::Region& region);

std::ostream& operator<<(std::ostream& out, RuleSet::SelectionType type);
std::istream& operator>>(std::istream& in, RuleSet::SelectionType& type);

std::ostream& operator<<(std::ostream& out, RuleSet::Rule::Status status);
std::istream& operator>>(std::istream& in, RuleSet::Rule::Status& status);

}  // namespace airmap

#endif  // AIRMAP_RULESET_H_
