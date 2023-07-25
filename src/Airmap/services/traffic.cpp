#include <Airmap/services/traffic.h>

std::shared_ptr<airmap::services::Traffic::Monitor> airmap::services::Traffic::Monitor::create(
    const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Traffic::Monitor>& native) {
  return std::shared_ptr<Monitor>{new Monitor{dispatcher, native}};
}

airmap::services::Traffic::Monitor::Monitor(const std::shared_ptr<Dispatcher>& dispatcher,
                                      const std::shared_ptr<airmap::Traffic::Monitor>& native)
    : dispatcher_{dispatcher}, native_{native} {
}

void airmap::services::Traffic::Monitor::subscribe(const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) {
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), subscriber] { sp->subscribers_.insert(subscriber); });
}

void airmap::services::Traffic::Monitor::unsubscribe(
    const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) {
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), subscriber] { sp->subscribers_.erase(subscriber); });
}

void airmap::services::Traffic::Monitor::handle_update(Update::Type type, const std::vector<Update>& update) {
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), type, update]() {
    for (const auto& subscriber : sp->subscribers_)
      subscriber->handle_update(type, update);
  });
}

std::shared_ptr<airmap::services::Traffic> airmap::services::Traffic::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                 const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Traffic>{new Traffic{dispatcher, client}};
}

airmap::services::Traffic::Traffic(const std::shared_ptr<Dispatcher>& dispatcher,
                             const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::services::Traffic::monitor(const Monitor::Params& parameters, const Monitor::Callback& cb) {
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
