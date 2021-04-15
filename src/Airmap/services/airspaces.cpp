<<<<<<< HEAD
#include <Airmap/services/airspaces.h>

std::shared_ptr<airmap::services::Airspaces> airmap::services::Airspaces::create(const std::shared_ptr<Dispatcher>& dispatcher,
=======
#include <Airmap/qt/airspaces.h>

std::shared_ptr<airmap::qt::Airspaces> airmap::qt::Airspaces::create(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                                     const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Airspaces>{new Airspaces{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::Airspaces::Airspaces(const std::shared_ptr<Dispatcher>& dispatcher,
=======
airmap::qt::Airspaces::Airspaces(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                 const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

<<<<<<< HEAD
void airmap::services::Airspaces::search(const Search::Parameters& parameters, const Search::Callback& cb) {
=======
void airmap::qt::Airspaces::search(const Search::Parameters& parameters, const Search::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->airspaces().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Airspaces::for_ids(const ForIds::Parameters& parameters, const ForIds::Callback& cb) {
=======
void airmap::qt::Airspaces::for_ids(const ForIds::Parameters& parameters, const ForIds::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->airspaces().for_ids(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
