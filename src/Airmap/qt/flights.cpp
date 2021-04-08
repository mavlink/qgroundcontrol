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
#include <Airmap/qt/flights.h>

std::shared_ptr<airmap::qt::Flights> airmap::qt::Flights::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                 const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<Flights>{new Flights{dispatcher, client}};
}

airmap::qt::Flights::Flights(const std::shared_ptr<Dispatcher>& dispatcher,
                             const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::qt::Flights::search(const Search::Parameters& parameters, const Search::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::qt::Flights::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::qt::Flights::create_flight_by_point(const CreateFlight::Parameters& parameters,
                                                 const CreateFlight::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().create_flight_by_point(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::qt::Flights::create_flight_by_path(const CreateFlight::Parameters& parameters,
                                                const CreateFlight::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().create_flight_by_path(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::qt::Flights::create_flight_by_polygon(const CreateFlight::Parameters& parameters,
                                                   const CreateFlight::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().create_flight_by_polygon(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::qt::Flights::end_flight(const EndFlight::Parameters& parameters, const EndFlight::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().end_flight(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::qt::Flights::delete_flight(const DeleteFlight::Parameters& parameters, const DeleteFlight::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().delete_flight(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::qt::Flights::start_flight_communications(const StartFlightCommunications::Parameters& parameters,
                                                      const StartFlightCommunications::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().start_flight_communications(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::qt::Flights::end_flight_communications(const EndFlightCommunications::Parameters& parameters,
                                                    const EndFlightCommunications::Callback& cb) {
  dispatcher_->dispatch_to_native([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->flights().end_flight_communications(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
