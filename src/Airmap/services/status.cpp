<<<<<<< HEAD
#include <Airmap/services/status.h>

std::shared_ptr<airmap::services::Status> airmap::services::Status::create(const std::shared_ptr<Dispatcher>& dispatcher,
=======
#include <Airmap/qt/status.h>

std::shared_ptr<airmap::qt::Status> airmap::qt::Status::create(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                               const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Status>{new Status{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::Status::Status(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::services::Status::get_status_by_point(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) {
=======
airmap::qt::Status::Status(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::qt::Status::get_status_by_point(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->status().get_status_by_point(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Status::get_status_by_path(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) {
=======
void airmap::qt::Status::get_status_by_path(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->status().get_status_by_path(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::Status::get_status_by_polygon(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) {
=======
void airmap::qt::Status::get_status_by_polygon(const GetStatus::Parameters& parameters, const GetStatus::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->status().get_status_by_polygon(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
