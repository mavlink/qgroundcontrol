#ifndef AIRMAP_AIRSPACES_H_
#define AIRMAP_AIRSPACES_H_

#include <airmap/airspace.h>
#include <airmap/date_time.h>
#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/outcome.h>

#include <functional>
#include <vector>

namespace airmap {

/// Airspaces provides functionality to query the airspace database.
class Airspaces : DoNotCopyOrMove {
 public:
  /// ForIds groups together types to ease interaction with
  /// Airspaces::ForIds.
  struct ForIds {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Airspace::Id id;  ///< Search for the airspace with this id.
    };

    /// Result models the outcome of calling Airspaces::for_id.
    using Result = Outcome<Airspace, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Airspaces::for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// Search groups together types to ease interaction with
  /// Airspaces::Search.
  struct Search {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Optional<Airspace::Type> types;          ///< Search for airspaces with either one of these types.
      Optional<Airspace::Type> ignored_types;  ///< Ignore airspaces with either one of these types.
      Optional<bool> full;  ///< If true, the complete description of airspaces in the result set is requested.
      Geometry geometry;    ///< Search airspaces intersection this geometry.
      Optional<std::uint32_t> buffer;  ///< Buffer around the geometry in [m].
      Optional<std::uint32_t> limit;   ///< Limit the number of results to 'limit'.
      Optional<std::uint32_t> offset;
      Optional<DateTime> date_time;
    };

    /// Result models the outcome of calling Airspaces::search.
    using Result = Outcome<std::vector<Airspace>, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Airspaces::search finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// search queries the AirMap services for surrounding airspaces and
  /// reports back the results to 'cb'.
  virtual void search(const Search::Parameters& parameters, const Search::Callback& cb) = 0;

  /// for_ids queries the AirMap services for detailed information about
  /// airspaces identified by UUIDs and reports back results to 'cb'.
  virtual void for_ids(const ForIds::Parameters& parameters, const ForIds::Callback& cb) = 0;

 protected:
  /// cond
  Airspaces() = default;
  /// @endcond
};

}  // namespace airmap

#endif  // AIRMAP_AIRSPACES_H_
