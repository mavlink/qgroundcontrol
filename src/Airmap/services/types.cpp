#include <Airmap/services/types.h>

namespace {

template <typename T>
inline void register_type_once(const char* name) {
  static const int id = qRegisterMetaType<T>(name);
  (void)id;
}

template <typename T>
inline void register_type_once() {
  static const int id = qRegisterMetaType<T>();
  (void)id;
}

}  // namespace

void airmap::services::register_types() {
  register_type_once<Aircraft>("Aircraft");
  register_type_once<Airspace>("Airspace");
  register_type_once<Credentials>("Credentials");
  register_type_once<DateTime>("DateTime");
  register_type_once<Error>("Error");
  register_type_once<FlightPlan>("FlightPlan");
  register_type_once<Flight>("Flight");
  register_type_once<Geometry>("Geometry");
  register_type_once<Pilot>("Pilot");
  register_type_once<Rule>("Rule");
  register_type_once<RuleSet>("RuleSet");
  register_type_once<RuleSet::Rule>("RuleSet::Rule");
  register_type_once<Status::Advisory>("Status::Advisory");
  register_type_once<Status::Wind>("Status::Wind");
  register_type_once<Status::Weather>("Status::Weather");
  register_type_once<Status::Report>("Status::Report");
  register_type_once<Telemetry::Position>("Telemetry::Position");
  register_type_once<Telemetry::Speed>("Telemetry::Speed");
  register_type_once<Telemetry::Attitude>("Telemetry::Attitude");
  register_type_once<Telemetry::Barometer>("Telemetry::Barometer");
  register_type_once<Optional<Telemetry::Update>>("Optional<Telemetry::Update>");
  register_type_once<Token::Type>("Token::Type");
  register_type_once<Token::Anonymous>("Token::Anonymous");
  register_type_once<Token::OAuth>("Token::OAuth");
  register_type_once<Token::Refreshed>("Token::Refreshed");
  register_type_once<Token>("Token");
  register_type_once<Traffic::Update::Type>("Traffic::Update::Type");
  register_type_once<Traffic::Update>("Traffic::Update");
  register_type_once<Version>("Version");

  register_type_once<airmap::Aircraft>("airmap::Aircraft");
  register_type_once<airmap::Airspace>("airmap::Airspace");
  register_type_once<airmap::Credentials>("airmap::Credentials");
  register_type_once<airmap::DateTime>("airmap::DateTime");
  register_type_once<airmap::Error>("airmap::Error");
  register_type_once<airmap::FlightPlan>("airmap::FlightPlan");
  register_type_once<airmap::Flight>("airmap::Flight");
  register_type_once<airmap::Geometry>("airmap::Geometry");
  register_type_once<airmap::Pilot>("airmap::Pilot");
  register_type_once<airmap::Rule>("airmap::Rule");
  register_type_once<airmap::RuleSet>("airmap::RuleSet");
  register_type_once<airmap::RuleSet::Rule>("airmap::RuleSet::Rule");
  register_type_once<airmap::Status::Advisory>("airmap::Advisory");
  register_type_once<airmap::Status::Wind>("airmap::Wind");
  register_type_once<airmap::Status::Weather>("airmap::Weather");
  register_type_once<airmap::Status::Report>("airmap::Report");
  register_type_once<airmap::Telemetry::Position>("airmap::Telemetry::Position");
  register_type_once<airmap::Telemetry::Speed>("airmap::Telemetry::Speed");
  register_type_once<airmap::Telemetry::Attitude>("airmap::Telemetry::Attitude");
  register_type_once<airmap::Telemetry::Barometer>("airmap::Telemetry::Barometer");
  register_type_once<airmap::Optional<airmap::Telemetry::Update>>("airmap::Optional<airmap::Telemetry::Update>");
  register_type_once<airmap::Token::Type>("airmap::Token::Type");
  register_type_once<airmap::Token::Anonymous>("airmap::Token::Anonymous");
  register_type_once<airmap::Token::OAuth>("airmap::Token::OAuth");
  register_type_once<airmap::Token::Refreshed>("airmap::Token::Refreshed");
  register_type_once<airmap::Token>("airmap::Token");
  register_type_once<airmap::Traffic::Update::Type>("airmap::Traffic::Update::Type");
  register_type_once<airmap::Traffic::Update>("airmap::Traffic::Update");
  register_type_once<airmap::Version>("airmap::Version");
}
