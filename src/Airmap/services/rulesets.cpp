#include <Airmap/services/rulesets.h>

std::shared_ptr<airmap::services::RuleSets> airmap::services::RuleSets::create(const std::shared_ptr<Dispatcher>& dispatcher,
                                                                   const std::shared_ptr<airmap::Client>& client) {
  return std::shared_ptr<RuleSets>{new RuleSets{dispatcher, client}};
}

airmap::services::RuleSets::RuleSets(const std::shared_ptr<Dispatcher>& dispatcher,
                               const std::shared_ptr<airmap::Client>& client)
    : dispatcher_{dispatcher}, client_{client} {
}

void airmap::services::RuleSets::search(const Search::Parameters& parameters, const Search::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().search(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::RuleSets::for_id(const ForId::Parameters& parameters, const ForId::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().for_id(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::RuleSets::fetch_rules(const FetchRules::Parameters& parameters, const FetchRules::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().fetch_rules(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::RuleSets::evaluate_rulesets(const EvaluateRules::Parameters& parameters,
                                             const EvaluateRules::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().evaluate_rulesets(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}

void airmap::services::RuleSets::evaluate_flight_plan(const EvaluateFlightPlan::Parameters& parameters,
                                                const EvaluateFlightPlan::Callback& cb) {
  dispatcher_->dispatch_to_airmap([this, sp = shared_from_this(), parameters, cb]() {
    sp->client_->rulesets().evaluate_flight_plan(parameters, [this, sp, cb](const auto& result) {
      sp->dispatcher_->dispatch_to_qt([sp, result, cb]() { cb(result); });
    });
  });
}
