#ifndef AIRMAP_PILOTS_H_
#define AIRMAP_PILOTS_H_

#include <airmap/aircraft.h>
#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/optional.h>
#include <airmap/outcome.h>
#include <airmap/pilot.h>

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace airmap {

/// Pilots provides functionality to manage (the authorized) pilot.
class Pilots : DoNotCopyOrMove {
 public:
  /// Exclude enumerates fields that can be excluded when querying pilot and aircraft properties.
  enum class Exclude {
    aircraft        = 1 << 0,  ///< Exclude aircraft data from results.
    user_metadata   = 1 << 1,  ///< Exclude user-specific metadata from results.
    app_metadata    = 1 << 2,  ///< Exclude app-specific metadata from results.
    authorized_apps = 1 << 3   ///< Exclude list of authorized apps from results.
  };

  /// Authenticated bundles up types to ease interaction
  /// with Pilots::authenticated.
  struct Authenticated {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;        ///< Authorization token obtained by logging in to the AirMap services.
      Optional<Exclude> exclude{};      ///< Exclude these fields from results.
      bool retrieve_statistics{false};  ///< If true, statistics about flights and aircrafts are requested.
    };

    /// Result models the outcome of calling Pilots::authenticated.
    using Result = Outcome<Pilot, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Pilots::authenticated finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// ForId bundles up types to ease interaction
  /// with Pilots::for_id.
  struct ForId {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;        ///< Authorization token obtained by logging in to the AirMap services.
      std::string id;                   ///< Searches for the specific pilot with this id.
      Optional<Exclude> exclude{};      ///< Exclude these fields from results.
      bool retrieve_statistics{false};  ///< If true, statistics about flights and aircrafts are requested.
    };

    /// Result models the outcome of calling Pilots::for_id.
    using Result = Outcome<Pilot, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Pilots::for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// UpdateForId bundles up types to ease interaction
  /// with Pilots::update_for_id.
  struct UpdateForId {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      std::string id;             ///< Updates the specific pilot with this id.
      std::string first_name;     ///< The first name of the pilot.
      std::string last_name;      ///< The last name of the pilot.
      std::string user_name;      ///< The AirMap username of this pilot.
      std::string phone;          ///< The phone number of the pilot.
      std::map<std::string, std::string> app_metadata;   ///< App-specific metadata associated to the pilot.
      std::map<std::string, std::string> user_metadata;  ///< User-specific metadata associated to the pilot.
    };

    /// Result models the outcome of calling Pilots::update_for_id.
    using Result = Outcome<Pilot, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Pilots::update_for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// StartVerifyPilotPhoneForId bundles up types to ease interaction
  /// with Pilots::start_verify_pilot_phone_for_id.
  struct StartVerifyPilotPhoneForId {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      std::string id;             ///< Verifies the phone number for the pilot with this id.
    };

    struct Empty {};

    /// Result models the outcome of calling Pilots::start_verify_pilot_phone_for_id.
    using Result = Outcome<Empty, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Pilots::start_verify_pilot_phone_for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// FinishVerifyPilotPhoneForId bundles up types to ease interaction
  /// with Pilots::finish_verify_pilot_phone_for_id.
  struct FinishVerifyPilotPhoneForId {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      std::string id;             ///< Verifies the phone number for the pilot with this id.
      std::uint32_t token;        ///< The token that was received on the pilot's phone.
    };

    struct Empty {};

    /// Result models the outcome of calling Pilots::finish_verify_pilot_phone_for_id.
    using Result = Outcome<Empty, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Pilots::finish_verify_pilot_phone_for_id finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// Aircrafts bundles up types to ease interaction
  /// with Pilots::aircrafts.
  struct Aircrafts {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      std::string id;             ///< Lists all aircrafts owned by the pilot with this id.
    };

    /// Result models the outcome of calling Pilots::aircrafts.
    using Result = Outcome<std::vector<Pilot::Aircraft>, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Pilots::aircrafts finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// AddAircraft bundles up types to ease interaction
  /// with Pilots::add_aircraft.
  struct AddAircraft {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      std::string id;             ///< Adds an aircraft for the pilot with this id.
      std::string model_id;       ///< The id of the model of the aircraft.
      std::string nick_name;      ///< The nickname of the aircraft.
    };

    /// Result models the outcome of calling Pilots::add_aircraft.
    using Result = Outcome<Pilot::Aircraft, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Pilots::add_aircraft finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// DeleteAircraft bundles up types to ease interaction
  /// with Pilots::delete_aircraft.
  struct DeleteAircraft {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      std::string id;             ///< Deletes an aircraft for the pilot with this id.
      std::string aircraft_id;    ///< Deletes the specific aircraft with this id.
    };

    struct Empty {};

    /// Result models the outcome of calling Pilots::delete_aircraft.
    using Result = Outcome<Empty, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Pilots::delete_aircraft finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// UpdateAircraft bundles up types to ease interaction
  /// with Pilots::update_aircraft.
  struct UpdateAircraft {
    /// Parameters bundles up input parameters.
    struct Parameters {
      std::string authorization;  ///< Authorization token obtained by logging in to the AirMap services.
      std::string id;             ///< Updates an aircraft for the pilot with this id.
      std::string aircraft_id;    ///< Update the specific aircraft with this id.
      std::string nick_name;      ///< The new nick name for the aircraft.
    };

    struct Empty {};
    /// Result models the outcome of calling Pilots::update_aircraft.
    using Result = Outcome<Empty, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Pilots::update_aircraft finishes.
    using Callback = std::function<void(const Result&)>;
  };

  /// current_user queries the AirMap services for the pilot profile
  /// connected to the authenticated user, reporting results to 'cb'.
  virtual void authenticated(const Authenticated::Parameters& parameters, const Authenticated::Callback& cb) = 0;

  /// for_id queries the AirMap services for the pilot profile
  /// with a given id, reporting results to 'cb'.
  virtual void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) = 0;

  /// update_for_id updates the pilot profile specified
  // by Parameters::id, reporting results to 'cb'.
  virtual void update_for_id(const UpdateForId::Parameters& parameters, const UpdateForId::Callback& cb) = 0;

  /// start_verify_pilot_phone_for_id sends a verification token to the phone
  /// number stored in the pilot profile, reporting results to 'cb'.
  virtual void start_verify_pilot_phone_for_id(const StartVerifyPilotPhoneForId::Parameters& parameters,
                                               const StartVerifyPilotPhoneForId::Callback& cb) = 0;

  /// finish_verify_pilot_phone_for_id responds to a verification request by
  /// sending back the token sent to the pilot's phone, reporting results to 'cb'.
  virtual void finish_verify_pilot_phone_for_id(const FinishVerifyPilotPhoneForId::Parameters& parameters,
                                                const FinishVerifyPilotPhoneForId::Callback& cb) = 0;

  /// aircrafts queries the list of aircrafts owned by a pilot, reporting results to 'cb'.
  virtual void aircrafts(const Aircrafts::Parameters& parameters, const Aircrafts::Callback& cb) = 0;

  /// add_aircraft associates a new aircraft with a pilot, reporting results to 'cb'.
  virtual void add_aircraft(const AddAircraft::Parameters& parameters, const AddAircraft::Callback& cb) = 0;

  /// delete_aircraft removes an aircraft from a pilot profile, reporting results to 'cb'.
  virtual void delete_aircraft(const DeleteAircraft::Parameters& parameters, const DeleteAircraft::Callback& cb) = 0;

  /// update_aircraft updates the properties of an aircraft associated with a pilot, reporting results to 'cb'.
  virtual void update_aircraft(const UpdateAircraft::Parameters& parameters, const UpdateAircraft::Callback& cb) = 0;

 protected:
  Pilots() = default;
};

/// @cond
Pilots::Exclude operator|(Pilots::Exclude, Pilots::Exclude);
Pilots::Exclude operator&(Pilots::Exclude, Pilots::Exclude);
std::ostream& operator<<(std::ostream& out, Pilots::Exclude exclude);
/// @endcond

}  // namespace airmap

#endif  // AIRMAP_PILOTS_H_
