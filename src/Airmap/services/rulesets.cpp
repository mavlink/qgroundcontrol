<<<<<<< HEAD
#include <Airmap/services/rulesets.h>

std::shared_ptr<airmap::services::RuleSets> airmap::services::RuleSets::create(const std::shared_ptr<Dispatcher>& dispatcher,
=======
#include <Airmap/qt/rulesets.h>

std::shared_ptr<airmap::qt::RuleSets> airmap::qt::RuleSets::create(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                                   const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<RuleSets>{new RuleSets{dispatcher, client}};
}

<<<<<<< HEAD
airmap::services::RuleSets::RuleSets(const std::shared_ptr<Dispatcher>& dispatcher,
=======
airmap::qt::RuleSets::RuleSets(const std::shared_ptr<Dispatcher>& dispatcher,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                               const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

<<<<<<< HEAD
void airmap::services::RuleSets::search(const Search::Parameters& parameters, const Search::Callback& cb) {
=======
void airmap::qt::RuleSets::search(const Search::Parameters& parameters, const Search::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::RuleSets::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
=======
void airmap::qt::RuleSets::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::RuleSets::fetch_rules(const FetchRules::Parameters& parameters, const FetchRules::Callback& cb) {
=======
void airmap::qt::RuleSets::fetch_rules(const FetchRules::Parameters& parameters, const FetchRules::Callback& cb) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().fetch_rules(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::RuleSets::evaluate_rulesets(const EvaluateRules::Parameters& parameters,
=======
void airmap::qt::RuleSets::evaluate_rulesets(const EvaluateRules::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                             const EvaluateRules::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().evaluate_rulesets(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

<<<<<<< HEAD
void airmap::services::RuleSets::evaluate_flight_plan(const EvaluateFlightPlan::Parameters& parameters,
=======
void airmap::qt::RuleSets::evaluate_flight_plan(const EvaluateFlightPlan::Parameters& parameters,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
                                                const EvaluateFlightPlan::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().evaluate_flight_plan(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
