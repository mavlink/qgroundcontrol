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
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), flight, key, updates]() {
    sp->client_->telemetry().submit_updates(flight, key, updates);
  });
}
