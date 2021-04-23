#include <Airmap/services/telemetry.h>

#include <airmap/flight.h>

std::shared_ptr<airmap::services::Telemetry> airmap::services::Telemetry::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                     const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Telemetry>{new Telemetry{dispatcher, client}};
}

airmap::services::Telemetry::Telemetry(const std::shared_ptr<Dispatcher>& dispatcher,
                                 const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::services::Telemetry::submit_updates(const Flight& flight, const std::string& key,
                                           const std::initializer_list<Update>& updates) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), flight, key, updates]() {
    sp->client_->telemetry().submit_updates(flight, key, updates);
  });
}
