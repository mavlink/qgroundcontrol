#include <Airmap/services/flights.h>

std::shared_ptr<airmap::services::Flights> airmap::services::Flights::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                 const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Flights>{new Flights{dispatcher, client}};
}

airmap::services::Flights::Flights(const std::shared_ptr<Dispatcher>& dispatcher,
                             const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::services::Flights::search(const Search::Parameters& parameters, const Search::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Flights::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Flights::create_flight_by_point(const CreateFlight::Parameters& parameters,
                                                 const CreateFlight::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().create_flight_by_point(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Flights::create_flight_by_path(const CreateFlight::Parameters& parameters,
                                                const CreateFlight::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().create_flight_by_path(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Flights::create_flight_by_polygon(const CreateFlight::Parameters& parameters,
                                                   const CreateFlight::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().create_flight_by_polygon(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Flights::end_flight(const EndFlight::Parameters& parameters, const EndFlight::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().end_flight(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Flights::delete_flight(const DeleteFlight::Parameters& parameters, const DeleteFlight::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().delete_flight(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Flights::start_flight_communications(const StartFlightCommunications::Parameters& parameters,
                                                      const StartFlightCommunications::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().start_flight_communications(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Flights::end_flight_communications(const EndFlightCommunications::Parameters& parameters,
                                                    const EndFlightCommunications::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().end_flight_communications(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
