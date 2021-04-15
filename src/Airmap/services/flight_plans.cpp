<<<<<<< HEAD
#include <Airmap/services/flight_plans.h>

std::shared_ptr<airmap::services::FlightPlans> airmap::services::FlightPlans::create(
=======
#include <Airmap/qt/flight_plans.h>

std::shared_ptr<airmap::qt::FlightPlans> airmap::qt::FlightPlans::create(
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
    const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<FlightPlans>{new FlightPlans{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::FlightPlans::FlightPlans(const std::shared_ptr<Dispatcher>& dispatcher,
=======
airmap::qt::FlightPlans::FlightPlans(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                     const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

<<<<<<< HEAD
void airmap::services::FlightPlans::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
=======
void airmap::qt::FlightPlans::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flight_plans().for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::FlightPlans::create_by_polygon(const Create::Parameters& parameters, const Create::Callback& cb) {
=======
void airmap::qt::FlightPlans::create_by_polygon(const Create::Parameters& parameters, const Create::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flight_plans().create_by_polygon(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::FlightPlans::update(const Update::Parameters& parameters, const Update::Callback& cb) {
=======
void airmap::qt::FlightPlans::update(const Update::Parameters& parameters, const Update::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flight_plans().update(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::FlightPlans::delete_(const Delete::Parameters& parameters, const Delete::Callback& cb) {
=======
void airmap::qt::FlightPlans::delete_(const Delete::Parameters& parameters, const Delete::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flight_plans().delete_(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::FlightPlans::render_briefing(const RenderBriefing::Parameters& parameters,
=======
void airmap::qt::FlightPlans::render_briefing(const RenderBriefing::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                              const RenderBriefing::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flight_plans().render_briefing(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::FlightPlans::submit(const Submit::Parameters& parameters, const Submit::Callback& cb) {
=======
void airmap::qt::FlightPlans::submit(const Submit::Parameters& parameters, const Submit::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flight_plans().submit(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
