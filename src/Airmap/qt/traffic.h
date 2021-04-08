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
#ifndef AIRMAP_QT_TRAFFIC_H_
#define AIRMAP_QT_TRAFFIC_H_

#include <airmap/traffic.h>

#include <Airmap/qt/dispatcher.h>

#include <memory>
#include <set>

namespace airmap {
namespace qt {

class Traffic : public airmap::Traffic, public std::enable_shared_from_this<Traffic> {
 public:
  static std::shared_ptr<Traffic> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                         const std::shared_ptr<airmap::Client>& client);

  class Monitor : public airmap::Traffic::Monitor,
                  public airmap::Traffic::Monitor::Subscriber,
                  public std::enable_shared_from_this<Monitor> {
   public:
    static std::shared_ptr<Monitor> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                           const std::shared_ptr<airmap::Traffic::Monitor>& native);

    // From airmap::Traffic::Monitor
    void subscribe(const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) override;
    void unsubscribe(const std::shared_ptr<airmap::Traffic::Monitor::Subscriber>& subscriber) override;
    // From airmap::Traffic::Monitor::Subscriber
    void handle_update(Update::Type type, const std::vector<Update>& update) override;

   private:
    explicit Monitor(const std::shared_ptr<Dispatcher>& dispatcher,
                     const std::shared_ptr<airmap::Traffic::Monitor>& native);

    std::shared_ptr<Dispatcher> dispatcher_;
    std::shared_ptr<airmap::Traffic::Monitor> native_;
    std::set<std::shared_ptr<airmap::Traffic::Monitor::Subscriber>> subscribers_;
  };

  // From airmap::Traffic
  void monitor(const Monitor::Params& params, const Monitor::Callback& cb) override;

 private:
  explicit Traffic(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);

  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_TRAFFIC_H_
