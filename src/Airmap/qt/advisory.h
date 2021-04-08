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
#ifndef AIRMAP_QT_ADVISORY_H_
#define AIRMAP_QT_ADVISORY_H_

#include <airmap/advisory.h>
#include <airmap/client.h>
#include "dispatcher.h"

namespace airmap {
namespace qt {

class Advisory : public airmap::Advisory, public std::enable_shared_from_this<Advisory> {
 public:
  static std::shared_ptr<Advisory> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                          const std::shared_ptr<airmap::Client>& client);

  void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) override;
  void search(const Search::Parameters& parameters, const Search::Callback& cb) override;
  void report_weather(const ReportWeather::Parameters& parameters, const ReportWeather::Callback& cb) override;

 private:
  explicit Advisory(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);
  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_ADVISORY_H_
