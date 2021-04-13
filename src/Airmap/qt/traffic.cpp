// AirMap Platform SDK
// Copyright © 2018 AirMap, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <Airmap/qt/traffic.h>

std::shared_ptr<airmap::qt::Traffic::Monitor> airmap::qt::Traffic::Monitor::create(
    const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Traffic::Monitor>& native) {
  return std::shared_ptr<Monitor>{new Monitor{dispatcher, native}};
}

airmap::qt::Traffic::Monitor::Monitor(const std::shared_ptr<Dispatcher>& dispatcher,
                                      const std::shared_ptr<airmap::Traffic::Monitor>& native)
    : dispatcher_{dispatcher}, native_{native} {
}

void airmap::qt::Traffic::Monitor::subscribe(const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) {
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), subscriber] { sp->subscribers_.insert(subscriber); });
}

void airmap::qt::Traffic::Monitor::unsubscribe(
    const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) {
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), subscriber] { sp->subscribers_.erase(subscriber); });
}

void airmap::qt::Traffic::Monitor::handle_update(Update::Type type, const std::vector<Update>& update) {
  dispatcher_->dispatch_to_qt([this, sp = shared_from_this(), type, update]() {
    for (const auto& subscriber : sp->subscribers_)
      subscriber->handle_update(type, update);
  });
}

std::shared_ptr<airmap::qt::Traffic> airmap::qt::Traffic::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                 const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Traffic>{new Traffic{dispatcher, client}};
}

airmap::qt::Traffic::Traffic(const std::shared_ptr<Dispatcher>& dispatcher,
                             const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::qt::Traffic::monitor(const Monitor::Params& parameters, const Monitor::Callback& cb) {
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
