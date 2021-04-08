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
#include <Airmap/qt/airspaces.h>

std::shared_ptr<airmap::qt::Airspaces> airmap::qt::Airspaces::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                     const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Airspaces>{new Airspaces{dispatcher, client}};
}

airmap::qt::Airspaces::Airspaces(const std::shared_ptr<Dispatcher>& dispatcher,
                                 const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::qt::Airspaces::search(const Search::Parameters& parameters, const Search::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->airspaces().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::qt::Airspaces::for_ids(const ForIds::Parameters& parameters, const ForIds::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->airspaces().for_ids(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
