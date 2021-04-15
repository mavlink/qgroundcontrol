<<<<<<< HEAD
#include <Airmap/services/flights.h>

std::shared_ptr<airmap::services::Flights> airmap::services::Flights::create(const std::shared_ptr<Dispatcher>& dispatcher,
=======
#include <Airmap/qt/flights.h>

std::shared_ptr<airmap::qt::Flights> airmap::qt::Flights::create(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                                 const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Flights>{new Flights{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::Flights::Flights(const std::shared_ptr<Dispatcher>& dispatcher,
=======
airmap::qt::Flights::Flights(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                             const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

<<<<<<< HEAD
void airmap::services::Flights::search(const Search::Parameters& parameters, const Search::Callback& cb) {
=======
void airmap::qt::Flights::search(const Search::Parameters& parameters, const Search::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Flights::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
=======
void airmap::qt::Flights::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Flights::create_flight_by_point(const CreateFlight::Parameters& parameters,
=======
void airmap::qt::Flights::create_flight_by_point(const CreateFlight::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                 const CreateFlight::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().create_flight_by_point(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Flights::create_flight_by_path(const CreateFlight::Parameters& parameters,
=======
void airmap::qt::Flights::create_flight_by_path(const CreateFlight::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                const CreateFlight::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().create_flight_by_path(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Flights::create_flight_by_polygon(const CreateFlight::Parameters& parameters,
=======
void airmap::qt::Flights::create_flight_by_polygon(const CreateFlight::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                   const CreateFlight::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().create_flight_by_polygon(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Flights::end_flight(const EndFlight::Parameters& parameters, const EndFlight::Callback& cb) {
=======
void airmap::qt::Flights::end_flight(const EndFlight::Parameters& parameters, const EndFlight::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().end_flight(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Flights::delete_flight(const DeleteFlight::Parameters& parameters, const DeleteFlight::Callback& cb) {
=======
void airmap::qt::Flights::delete_flight(const DeleteFlight::Parameters& parameters, const DeleteFlight::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().delete_flight(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Flights::start_flight_communications(const StartFlightCommunications::Parameters& parameters,
=======
void airmap::qt::Flights::start_flight_communications(const StartFlightCommunications::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                      const StartFlightCommunications::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().start_flight_communications(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Flights::end_flight_communications(const EndFlightCommunications::Parameters& parameters,
=======
void airmap::qt::Flights::end_flight_communications(const EndFlightCommunications::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                    const EndFlightCommunications::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().end_flight_communications(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
