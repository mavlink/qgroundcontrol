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
#ifndef AIRMAP_QT_RULESETS_H_
#define AIRMAP_QT_RULESETS_H_

#include <airmap/client.h>
#include <Airmap/qt/dispatcher.h>
#include <airmap/rulesets.h>

namespace airmap {
namespace qt {

class RuleSets : public airmap::RuleSets, public std::enable_shared_from_this<RuleSets> {
 public:
  static std::shared_ptr<RuleSets> create(const std::shared_ptr<Dispatcher>& dispatcher,
                                          const std::shared_ptr<airmap::Client>& client);

  void search(const Search::Parameters& parameters, const Search::Callback& cb) override;
  void for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) override;
  void fetch_rules(const FetchRules::Parameters& parameters, const FetchRules::Callback& cb) override;
  void evaluate_rulesets(const EvaluateRules::Parameters& parameters, const EvaluateRules::Callback& cb) override;
  void evaluate_flight_plan(const EvaluateFlightPlan::Parameters& parameters, const EvaluateFlightPlan::Callback& cb) override;

 private:
  explicit RuleSets(const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client);
  std::shared_ptr<Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_RULESETS_H_
