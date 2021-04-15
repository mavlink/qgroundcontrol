<<<<<<< HEAD
#include <Airmap/services/advisory.h>

std::shared_ptr<airmap::services::Advisory> airmap::services::Advisory::create(const std::shared_ptr<Dispatcher>& dispatcher,
=======
#include <Airmap/qt/advisory.h>

std::shared_ptr<airmap::qt::Advisory> airmap::qt::Advisory::create(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                                   const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Advisory>{new Advisory{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::Advisory::Advisory(const std::shared_ptr<Dispatcher>& dispatcher,
=======
airmap::qt::Advisory::Advisory(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                               const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

<<<<<<< HEAD
void airmap::services::Advisory::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
=======
void airmap::qt::Advisory::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->advisory().for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Advisory::search(const Search::Parameters& parameters, const Search::Callback& cb) {
=======
void airmap::qt::Advisory::search(const Search::Parameters& parameters, const Search::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->advisory().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Advisory::report_weather(const ReportWeather::Parameters& parameters,
=======
void airmap::qt::Advisory::report_weather(const ReportWeather::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                          const ReportWeather::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->advisory().report_weather(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
