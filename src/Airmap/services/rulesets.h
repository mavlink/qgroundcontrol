#ifndef AIRMAP_QT_RULESETS_H_
#define AIRMAP_QT_RULESETS_H_

#include <airmap/client.h>
#include <Airmap/services/dispatcher.h>
#include <airmap/rulesets.h>

namespace airmap {
namespace services {

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
