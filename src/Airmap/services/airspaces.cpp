#include <Airmap/services/airspaces.h>

std::shared_ptr<airmap::services::Airspaces> airmap::services::Airspaces::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                     const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Airspaces>{new Airspaces{dispatcher, client}};
}

airmap::services::Airspaces::Airspaces(const std::shared_ptr<Dispatcher>& dispatcher,
                                 const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::services::Airspaces::search(const Search::Parameters& parameters, const Search::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->airspaces().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Airspaces::for_ids(const ForIds::Parameters& parameters, const ForIds::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->airspaces().for_ids(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
