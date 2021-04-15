<<<<<<< HEAD
#include <Airmap/services/pilots.h>

std::shared_ptr<airmap::services::Pilots> airmap::services::Pilots::create(const std::shared_ptr<Dispatcher>& dispatcher,
=======
#include <Airmap/qt/pilots.h>

std::shared_ptr<airmap::qt::Pilots> airmap::qt::Pilots::create(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                               const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Pilots>{new Pilots{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::Pilots::Pilots(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::services::Pilots::authenticated(const Authenticated::Parameters& parameters, const Authenticated::Callback& cb) {
=======
airmap::qt::Pilots::Pilots(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::qt::Pilots::authenticated(const Authenticated::Parameters& parameters, const Authenticated::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->pilots().authenticated(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Pilots::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
=======
void airmap::qt::Pilots::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->pilots().for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Pilots::update_for_id(const UpdateForId::Parameters& parameters, const UpdateForId::Callback& cb) {
=======
void airmap::qt::Pilots::update_for_id(const UpdateForId::Parameters& parameters, const UpdateForId::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->pilots().update_for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Pilots::start_verify_pilot_phone_for_id(const StartVerifyPilotPhoneForId::Parameters& parameters,
=======
void airmap::qt::Pilots::start_verify_pilot_phone_for_id(const StartVerifyPilotPhoneForId::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                         const StartVerifyPilotPhoneForId::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->pilots().start_verify_pilot_phone_for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Pilots::finish_verify_pilot_phone_for_id(const FinishVerifyPilotPhoneForId::Parameters& parameters,
=======
void airmap::qt::Pilots::finish_verify_pilot_phone_for_id(const FinishVerifyPilotPhoneForId::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                          const FinishVerifyPilotPhoneForId::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->pilots().finish_verify_pilot_phone_for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Pilots::aircrafts(const Aircrafts::Parameters& parameters, const Aircrafts::Callback& cb) {
=======
void airmap::qt::Pilots::aircrafts(const Aircrafts::Parameters& parameters, const Aircrafts::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->pilots().aircrafts(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Pilots::add_aircraft(const AddAircraft::Parameters& parameters, const AddAircraft::Callback& cb) {
=======
void airmap::qt::Pilots::add_aircraft(const AddAircraft::Parameters& parameters, const AddAircraft::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->pilots().add_aircraft(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Pilots::delete_aircraft(const DeleteAircraft::Parameters& parameters,
=======
void airmap::qt::Pilots::delete_aircraft(const DeleteAircraft::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                         const DeleteAircraft::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->pilots().delete_aircraft(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Pilots::update_aircraft(const UpdateAircraft::Parameters& parameters,
=======
void airmap::qt::Pilots::update_aircraft(const UpdateAircraft::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                         const UpdateAircraft::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->pilots().update_aircraft(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
