#ifndef AIRMAP_FLIGHT_PLANS_H_
#define AIRMAP_FLIGHT_PLANS_H_

#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/flight_plan.h>
#include <airmap/outcome.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace airmap {

/// FlightPlans provides functionality for managing flight plans.
class FlightPlans : DoNotCopyOrMove {
 public:
  /// ForId bundles up types to ease interaction with
  /// FlightPlans::for_id.
  struct ForId {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Optional<std::string> authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      FlightPlan::Id id;                    ///< Search for the flight with this id.
    };

    /// Result models the outcome of calling FlightPlans::for_id.
    using Result = Outcome<FlightPlan, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to FlightPlans::for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// Create bundles up types to ease interaction with
  /// FlightPlans::create_by_point and FlightPlans::create_by_polygon.
  struct Create {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;           ///< Authorization token obtained by logging in to the AirMap services.
      Pilot pilot;                         ///< The pilot responsible for the flight.
      Optional<Pilot::Aircraft> aircraft;  ///< The aircraft conducting the flight.
      float latitude;                      ///< The latitude component of the takeoff point in [°].
      float longitude;                     ///< The longitude component of the takeoff point in [°].
      float max_altitude;                  ///< The maximum altitude over the entire flight in [m].
      float min_altitude;                  ///< The minimum altitude over the entire flight in [m].
      float buffer;                        ///< The buffer in [m] around the geometry.
      Geometry geometry;                   ///< The geometry describing the flight.
      DateTime start_time;                 ///< Point in time when the flight will start/was started.
      DateTime end_time;                   ///< Point in time when the fligth will end.
      std::vector<RuleSet::Id> rulesets;   ///< RuleSets that apply to this flight plan.
      std::unordered_map<std::string, RuleSet::Feature::Value>
          features;  ///< Additional properties of the planned flight.
    };

    /// Result models the outcome of calling FlightPlans::create_by_polygon.
    using Result = Outcome<FlightPlan, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to FlightPlans::create_by_point or FlightPlans::create_by_polygon finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// Update bundles up types to ease interaction with
  /// FlightPlans::update.
  struct Update {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Optional<std::string> authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      FlightPlan flight_plan;  ///< The details of the plan that should be created with the AirMap services.
    };
    /// Result models the outcome of calling FlightPlans::update.
    using Result = Outcome<FlightPlan, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to FlightPlans::update finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// Delete bundles up types to ease interaction with
  /// FlightPlans::delete_.
  struct Delete {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Optional<std::string> authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      FlightPlan::Id id;                    ///< Id of the flight plan that should be deleted.
    };

    /// Response models the response from the AirMap services.
    struct Response {
      FlightPlan::Id id;  ///< Id of the flight plan that was deleted.
    };

    /// Result models the outcome of calling FlightPlans::delete_flight.
    using Result = Outcome<Response, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to FlightPlans::delete_flight finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// RenderBriefing bundles up types to ease interaction with
  /// FlightPlans::render_briefing.
  struct RenderBriefing {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Optional<std::string> authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      FlightPlan::Id id;                    ///< Id of the flight plan that should be rendered as a briefing.
    };
    /// Result models the outcome of calling FlightPlans::submit.
    using Result = Outcome<FlightPlan::Briefing, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to FlightPlans::submit finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// Submit bundles up types to ease interaction with
  /// FlightPlans::submit.
  struct Submit {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Optional<std::string> authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      FlightPlan::Id id;                    ///< Id of the flight plan that should be submitted.
    };
    /// Result models the outcome of calling FlightPlans::submit.
    using Result = Outcome<FlightPlan, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to FlightPlans::submit finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// for_id queries the AirMap services for detailed information about
  /// a flight plan identified by a UUID and reports back results to 'cb'.
  virtual void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) = 0;

  /// create_by_polygon creates a flight plan for 'parameters' and reports
  /// results back to 'cb'.
  virtual void create_by_polygon(const Create::Parameters& parameters, const Create::Callback& cb) = 0;

  /// update updates a flight plan identified by 'parameters' and reports
  /// results back to 'cb'.
  virtual void update(const Update::Parameters& parameters, const Update::Callback& cb) = 0;

  /// delete deletes a flight plan identified by 'parameters' and reports
  /// results back to 'cb'.
  virtual void delete_(const Delete::Parameters& parameters, const Delete::Callback& cb) = 0;

  /// render_briefing requests rendering a briefing for a flight plan identified by 'parameters' and reports
  /// results back to 'cb'.
  virtual void render_briefing(const RenderBriefing::Parameters& parameters, const RenderBriefing::Callback& cb) = 0;

  /// submit submits a flight plan identified by 'parameters' and reports
  /// results back to 'cb'.
  virtual void submit(const Submit::Parameters& parameters, const Submit::Callback& cb) = 0;

 protected:
  /// @cond
  FlightPlans() = default;
  /// @endcond
};

}  // namespace airmap

#endif  // AIRMAP_FLIGHT_PLANS_H_
