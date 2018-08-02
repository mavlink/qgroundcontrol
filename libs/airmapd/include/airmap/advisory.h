#ifndef AIRMAP_ADVISORY_H_
#define AIRMAP_ADVISORY_H_

#include <airmap/airspace.h>
#include <airmap/date_time.h>
#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/flight_plan.h>
#include <airmap/geometry.h>
#include <airmap/optional.h>
#include <airmap/outcome.h>
#include <airmap/ruleset.h>
#include <airmap/status.h>

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace airmap {

/// Advisory provides functionality to query airspace and weather information about
/// a geographic area.
class Advisory : DoNotCopyOrMove {
 public:
  /// Advisory bundles together airspace information and its evaluation in terms
  /// good to fly/needs information or feedback/conflict.
  struct AirspaceAdvisory {
    Status::Advisory advisory;  /// Airspace information.
    Status::Color color;        /// The evaluation of the airspace.
    std::uint32_t rule_id;      /// The id of the ruleset.
    std::string ruleset_id;     /// The id of the rule.
  };

  /// Wind bundles up attributes describing a wind conditions.
  struct Wind {
    std::uint32_t heading = 0;    ///< The heading in [°].
    float speed           = 0.0;  ///< The speed in [°].
    std::uint32_t gusting = 0;
  };

  /// Weather bundles up attributes describing a weather condition.
  struct Weather {
    std::string condition;      ///< The overall weather condition.
    std::string icon;           ///< The icon or class of icon that should be used for display purposes.
    Wind wind;                  ///< The details about the current wind conditions.
    float temperature   = 0.0;  ///< The temperature in [°C].
    float humidity      = 0.0;
    float visibility    = 0.0;  ///< Visibility in [m].
    float precipitation = 0.0;  ///< The probability of precipitation in [%].
    std::string timezone;       ///< The timezone of the weather location.
    DateTime time;              ///< Timestamp of the weather report.
    float dew_point = 0.0;      ///< The current dew point.
    float mslp      = 0.0;      ///< The Median Sea Level Pressure in [mbar].
  };

  /// ForId bundles up types to ease interaction
  /// with Advisory::for_id.
  struct ForId {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Optional<DateTime> start;  ///< Search for advisories before this time.
      Optional<DateTime> end;    ///< Search for advisories after this time.
      FlightPlan::Id id;         ///< Search for advisories relating to this flight plan.
    };
    /// Result models the outcome of calling Advisory::for_id.
    using Result = Outcome<std::vector<AirspaceAdvisory>, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Advisory::for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// Search bundles up types to ease interaction
  /// with Advisory::search.
  struct Search {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Required<Geometry> geometry;     ///< Evaluate rulesets intersecting this geometry.
      Required<std::string> rulesets;  ///< Evaluate these rulesets.
      Optional<DateTime> start;        ///< Search for advisories after this time.
      Optional<DateTime> end;          ///< Search for advisories before this time.
    };
    /// Result models the outcome of calling Advisory::search.
    using Result = Outcome<std::vector<AirspaceAdvisory>, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Advisory::_search finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// ReportWeather bundles up types to ease interaction
  /// with Advisory::report_weather.
  struct ReportWeather {
    /// Parameters bundles up input parameters.
    struct Parameters {
      float latitude;            ///< The latitude component of the takeoff point in [°].
      float longitude;           ///< The longitude component of the takeoff point in [°].
      Optional<DateTime> start;  ///< Search for weather data after this time.
      Optional<DateTime> end;    ///< Search for weather data before this time.
    };
    /// Result models the outcome of calling Advisory::report_weather.
    using Result = Outcome<Weather, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Advisory::report_weather finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// for_id searches flight advisories for a flight plan and reports
  /// results back to 'cb'.
  virtual void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) = 0;

  /// search searches flight advisories for 'parameters' and reports
  /// results back to 'cb'.
  virtual void search(const Search::Parameters& parameters, const Search::Callback& cb) = 0;

  /// report_weather gets the current weather conditions and reports
  /// results back to 'cb'.
  virtual void report_weather(const ReportWeather::Parameters& parameters, const ReportWeather::Callback& cb) = 0;

 protected:
  /// @cond
  Advisory() = default;
  /// @endcond
};

}  // namespace airmap

#endif  // AIRMAP_ADVISORY_H_
