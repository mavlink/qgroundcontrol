#ifndef AIRMAP_FLIGHTS_H_
#define AIRMAP_FLIGHTS_H_

#include <airmap/date_time.h>
#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/flight.h>
#include <airmap/geometry.h>
#include <airmap/optional.h>
#include <airmap/outcome.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace airmap {

/// Flights provides functionality for managing flights.
class Flights : DoNotCopyOrMove {
 public:
  /// ForId bundles up types to ease interaction with
  /// Flights::for_id.
  struct ForId {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Optional<std::string> authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      Flight::Id id;                        ///< Search for the flight with this id.
      Optional<bool> enhance;               ///< If true, provides extended information per flight in the result set.
    };

    /// Result models the outcome of calling Flights::for_id.
    using Result = Outcome<Flight, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to Flights::for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// Search bundles up types to ease interaction with
  /// Flights::search.
  struct Search {
    /// Parameters bundles up input parameters.
    struct Parameters {
      Optional<std::string> authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      Optional<std::uint32_t> limit;        ///< Limit the number of results to 'limit'.
      Optional<Geometry> geometry;          ///< Search for flights intersecting this geometry.
      Optional<std::string> country;        ///< Search for flights in this country.
      Optional<std::string> state;          ///< Search for flights in this state.
      Optional<std::string> city;           ///< Search for flights in this city.
      Optional<std::string> pilot_id;       ///< Search for flights operated by this pilot.
      Optional<DateTime> start_after;       ///< Search for flights that started after this timestamp.
      Optional<DateTime> start_before;      ///< Search for flights that started before this timestamp.
      Optional<DateTime> end_after;         ///< Search for flights that ended after this timestamp.
      Optional<DateTime> end_before;        ///< Search for flights that ended before this timestamp.
      Optional<bool> enhance;               ///< If true, provides extended information per flight in the result set.
    };

    /// Response bundles up pagination and actual results for a call to Flights::search.
    struct Response {
      struct Paging {
        std::uint32_t limit;        ///< The maximum number of results per page.
        std::uint32_t total;        ///< The total number of results.
      } paging;                     ///< Bundles up pagination information.
      std::vector<Flight> flights;  ///< One page of flight results.
    };

    /// Result models the outcome of calling Flights::search.
    using Result = Outcome<Response, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to Flights::search finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// CreateFlight bundles up types to ease interaction with
  /// Flights::create_flight_by_point, Flights::create_flight_by_path and
  /// Flights::create_flight_by_polygon.
  struct CreateFlight {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;        ///< Authorization token obtained by logging in to the AirMap services.
      Required<float> latitude;         ///< Latitude of take-off point in [°].
      Required<float> longitude;        ///< Longitude of take-off point in [°].
      float max_altitude = 121.;        ///< Maximum altitude of the entire flight in [m].
      std::string aircraft_id;          ///< Id of the aircraft carrying out the flight.
      DateTime start_time;              ///< Point in time when the flight started.
      DateTime end_time;                ///< Point in time when the flight will end.
      bool is_public           = true;  ///< If true, the flight is considered public and displayed to AirMap users.
      bool give_digital_notice = true;  ///< If true, the flight is announced to airspace operators.
      float buffer             = 100;   ///< Buffer around the take-off point in [m].
      Optional<Geometry> geometry;      ///< The geometry that describes the flight.
    };
    /// Result models the outcome of calling Flights::create_flight.
    using Result = Outcome<Flight, Error>;
    /// Callback describes the function signature of the callback that is invoked
    /// when a call to Flights::create_flight finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// DeleteFlight bundles up types to ease interaction with
  /// Flights::delete_flight.
  struct DeleteFlight {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      Flight::Id id;              ///< Id of the flight that should be deleted.
    };

    /// Response models the response from the AirMap services.
    struct Response {
      Flight::Id id;  ///< Id of the flight that was deleted.
    };

    /// Result models the outcome of calling Flights::delete_flight.
    using Result = Outcome<Response, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Flights::delete_flight finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// EndFlight bundles up types to ease interaction with
  /// Flights::end_flight.
  struct EndFlight {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      Flight::Id id;              ///< Id of the flight that should be ended.
    };

    /// Response models the response from the AirMap services.
    struct Response {
      DateTime end_time;  ///< Point in time when the flight was ended.
    };

    /// Result models the outcome of calling Flights::delete_flight.
    using Result = Outcome<Response, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Flights::end_flight finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// StartFlightCommunications bundles up types to ease interaction with
  /// Flights::start_flight_communications.
  struct StartFlightCommunications {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      Flight::Id id;              ///< Id of the flight for which flight comms should be started.
    };

    /// Response models the response from the AirMap services.
    struct Response {
      std::string key;  ///< The encryption key that should be used to encrypt individual telemetry updates.
    };

    /// Result models the outcome of calling Flights::start_flight_communications.
    using Result = Outcome<Response, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Flights::start_flight_communications.
    using Callback = std::function<void(const Result&)>;
  };

  /// EndFlightCommunications bundles up types to ease interaction with
  /// Flights::end_flight_communications.
  struct EndFlightCommunications {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      Flight::Id id;              ///< Id of the flight for which flight comms should be ended.
    };

    /// Response models the response from the AirMap services.
    struct Response {};

    /// Result models the outcome of calling Flights::end_flight_communications.
    using Result = Outcome<Response, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Flights::end_flight_communications finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// search queries the AirMap services for known flights
  /// and reports results to 'cb'.
  virtual void search(const Search::Parameters& parameters, const Search::Callback& cb) = 0;

  /// for_ids queries the AirMap services for detailed information about
  /// flights identified by UUIDs and reports back results to 'cb'.
  virtual void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) = 0;

  /// create_flight creates a flight for 'parameters' and reports
  /// results back to 'cb'.
  virtual void create_flight_by_point(const CreateFlight::Parameters& parameters, const CreateFlight::Callback& cb) = 0;

  /// create_flight creates a flight for 'parameters' and reports
  /// results back to 'cb'.
  virtual void create_flight_by_path(const CreateFlight::Parameters& parameters, const CreateFlight::Callback& cb) = 0;

  /// create_flight creates a flight for 'parameters' and reports
  /// results back to 'cb'.
  virtual void create_flight_by_polygon(const CreateFlight::Parameters& parameters,
                                        const CreateFlight::Callback& cb) = 0;

  /// end_flight finalizes a flight identified by 'parameters' and reports
  /// results back to 'cb'.
  virtual void end_flight(const EndFlight::Parameters& parameters, const EndFlight::Callback& cb) = 0;

  /// delete_flight deletes a flight identified by 'parameters' and reports
  /// results back to 'cb'.
  virtual void delete_flight(const DeleteFlight::Parameters& parameters, const DeleteFlight::Callback& cb) = 0;

  /// start_flight_communications enables communications for a specific flight
  /// instance and reports results back to 'cb'.
  virtual void start_flight_communications(const StartFlightCommunications::Parameters& parameters,
                                           const StartFlightCommunications::Callback& cb) = 0;

  /// end_flight_communications enables communications for a specific flight
  /// instance and reports results back to cb.
  virtual void end_flight_communications(const EndFlightCommunications::Parameters& parameters,
                                         const EndFlightCommunications::Callback& cb) = 0;

 protected:
  /// @cond
  Flights() = default;
  /// @endcond
};

}  // namespace airmap

#endif  // AIRMAP_AIRSPACES_H_
