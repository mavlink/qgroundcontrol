<<<<<<< HEAD
#include <Airmap/services/aircrafts.h>

std::shared_ptr<airmap::services::Aircrafts> airmap::services::Aircrafts::create(const std::shared_ptr<Dispatcher>& dispatcher,
=======
#include <Airmap/qt/aircrafts.h>

std::shared_ptr<airmap::qt::Aircrafts> airmap::qt::Aircrafts::create(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                                     const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Aircrafts>{new Aircrafts{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::Aircrafts::Aircrafts(const std::shared_ptr<Dispatcher>& dispatcher,
=======
airmap::qt::Aircrafts::Aircrafts(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                 const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

<<<<<<< HEAD
void airmap::services::Aircrafts::manufacturers(const Manufacturers::Parameters& parameters,
=======
void airmap::qt::Aircrafts::manufacturers(const Manufacturers::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                          const Manufacturers::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->aircrafts().manufacturers(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Aircrafts::models(const Models::Parameters& parameters, const Models::Callback& cb) {
=======
void airmap::qt::Aircrafts::models(const Models::Parameters& parameters, const Models::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->aircrafts().models(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Aircrafts::model_for_id(const ModelForId::Parameters& parameters, const ModelForId::Callback& cb) {
=======
void airmap::qt::Aircrafts::model_for_id(const ModelForId::Parameters& parameters, const ModelForId::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->aircrafts().model_for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
