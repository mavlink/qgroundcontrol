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
#ifndef AIRMAP_QT_FLIGHTS_H_
#define AIRMAP_QT_FLIGHTS_H_

#include <airmap/client.h>
#include <airmap/flights.h>
#include <Airmap/qt/dispatcher.h>

#include <memory>

namespace airmap {
namespace qt {

class Flights : public airmap::Flights, public std::enable_shared_from_this<Flights> {
 public:
  static std::shared_ptr<Flights> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                         const std::shared_ptr<airmap::Client>& client);

  void search(const Search::Parameters& parameters, const Search::Callback& cb) override;
  void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) override;
  void create_flight_by_point(const CreateFlight::Parameters& parameters, const CreateFlight::Callback& cb) override;
  void create_flight_by_path(const CreateFlight::Parameters& parameters, const CreateFlight::Callback& cb) override;
  void create_flight_by_polygon(const CreateFlight::Parameters& parameters, const CreateFlight::Callback& cb) override;
  void end_flight(const EndFlight::Parameters& parameters, const EndFlight::Callback& cb) override;
  void delete_flight(const DeleteFlight::Parameters& parameters, const DeleteFlight::Callback& cb) override;
  void start_flight_communications(const StartFlightCommunications::Parameters& parameters,
                                   const StartFlightCommunications::Callback& cb) override;
  void end_flight_communications(const EndFlightCommunications::Parameters& parameters,
                                 const EndFlightCommunications::Callback& cb) override;

 private:
  explicit Flights(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_FLIGHTS_H_
