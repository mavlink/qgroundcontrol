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
#ifndef AIRMAP_QT_AIRSPACES_H_
#define AIRMAP_QT_AIRSPACES_H_

#include <airmap/airspaces.h>
#include <airmap/client.h>
#include <Airmap/qt/dispatcher.h>

#include <memory>

namespace airmap {
namespace qt {

class Airspaces : public airmap::Airspaces, public std::enable_shared_from_this<Airspaces> {
 public:
  static std::shared_ptr<Airspaces> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                           const std::shared_ptr<airmap::Client>& client);

  void search(const Search::Parameters& parameters, const Search::Callback& cb) override;
  void for_ids(const ForIds::Parameters& parameters, const ForIds::Callback& cb) override;

 private:
  explicit Airspaces(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_AIRSPACES_H_
