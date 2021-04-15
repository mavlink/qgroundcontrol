#include <Airmap/qt/telemetry.h>

#include <airmap/flight.h>

std::shared_ptr<airmap::qt::Telemetry> airmap::qt::Telemetry::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                     const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Telemetry>{new Telemetry{dispatcher, client}};
}

airmap::qt::Telemetry::Telemetry(const std::shared_ptr<Dispatcher>& dispatcher,
                                 const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::qt::Telemetry::submit_updates(const Flight& flight, const std::string& key,
                                           const std::initializer_list<Update>& updates) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), flight, key, updates]() {
    sp->client_->telemetry().submit_updates(flight, key, updates);
  });
}
