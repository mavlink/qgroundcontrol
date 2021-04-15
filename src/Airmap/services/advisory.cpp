#include <Airmap/services/advisory.h>

std::shared_ptr<airmap::services::Advisory> airmap::services::Advisory::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                   const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Advisory>{new Advisory{dispatcher, client}};
}

airmap::services::Advisory::Advisory(const std::shared_ptr<Dispatcher>& dispatcher,
                               const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::services::Advisory::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->advisory().for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Advisory::search(const Search::Parameters& parameters, const Search::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->advisory().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::Advisory::report_weather(const ReportWeather::Parameters& parameters,
                                          const ReportWeather::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->advisory().report_weather(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
