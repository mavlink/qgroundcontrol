<<<<<<< HEAD
#include <Airmap/services/traffic.h>

std::shared_ptr<airmap::services::Traffic::Monitor> airmap::services::Traffic::Monitor::create(
=======
#include <Airmap/qt/traffic.h>

std::shared_ptr<airmap::qt::Traffic::Monitor> airmap::qt::Traffic::Monitor::create(
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
    const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Traffic::Monitor>& native) {
  return std::shared_ptr<Monitor>{new Monitor{dispatcher, native}};
}

<<<<<<< HEAD
airmap::services::Traffic::Monitor::Monitor(const std::shared_ptr<Dispatcher>& dispatcher,
=======
airmap::qt::Traffic::Monitor::Monitor(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                      const std::shared_ptr<airmap::Traffic::Monitor>& native)
    : dispatcher_{dispatcher}, native_{native} {
}

<<<<<<< HEAD
void airmap::services::Traffic::Monitor::subscribe(const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) {
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), subscriber] { sp->subscribers_.insert(subscriber); });
}

void airmap::services::Traffic::Monitor::unsubscribe(
=======
void airmap::qt::Traffic::Monitor::subscribe(const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) {
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), subscriber] { sp->subscribers_.insert(subscriber); });
}

void airmap::qt::Traffic::Monitor::unsubscribe(
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
    const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) {
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), subscriber] { sp->subscribers_.erase(subscriber); });
}

<<<<<<< HEAD
void airmap::services::Traffic::Monitor::handle_update(Update::Type type, const std::vector<Update>& update) {
=======
void airmap::qt::Traffic::Monitor::handle_update(Update::Type type, const std::vector<Update>& update) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), type, update]() {
    for (const auto& subscriber : sp->subscribers_)
      subscriber->handle_update(type, update);
  });
}

<<<<<<< HEAD
std::shared_ptr<airmap::services::Traffic> airmap::services::Traffic::create(const std::shared_ptr<Dispatcher>& dispatcher,
=======
std::shared_ptr<airmap::qt::Traffic> airmap::qt::Traffic::create(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                                 const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Traffic>{new Traffic{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::Traffic::Traffic(const std::shared_ptr<Dispatcher>& dispatcher,
=======
airmap::qt::Traffic::Traffic(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                             const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

<<<<<<< HEAD
void airmap::services::Traffic::monitor(const Monitor::Params& parameters, const Monitor::Callback& cb) {
=======
void airmap::qt::Traffic::monitor(const Monitor::Params& parameters, const Monitor::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->traffic().monitor(parameters, [this, sp, parameters, cb](const auto& result) {
      if (result) {
        auto m  = result.value();
        auto mm = Monitor::create(sp->dispatcher_, m);
        m->subscribe(mm);
        sp->dispatcher_->dispatch_to_qt([sp, mm, cb]() { cb(Monitor::Result{mm}); });
      } else {
        sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
      }
    });
  });
}
