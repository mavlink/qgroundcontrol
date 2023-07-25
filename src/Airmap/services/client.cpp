#include <Airmap/services/client.h>

#include <Airmap/services/dispatcher.h>

#include <Airmap/services/advisory.h>
#include <Airmap/services/aircrafts.h>
#include <Airmap/services/airspaces.h>
#include <Airmap/services/authenticator.h>
#include <Airmap/services/flight_plans.h>
#include <Airmap/services/flights.h>
#include <Airmap/services/pilots.h>
#include <Airmap/services/rulesets.h>
#include <Airmap/services/status.h>
#include <Airmap/services/telemetry.h>
#include <Airmap/services/traffic.h>
#include <Airmap/services/types.h>

#include <memory>
#include <thread>

namespace {

class ContextRunner {
 public:
  explicit ContextRunner(const std::shared_ptr<airmap::Context>& context) : context_{context} {
  }

  void start() {
    worker_ = std::thread{[this]() { context_->run(); }};
  }

  void stop() {
    context_->stop();
    if (worker_.joinable())
      worker_.join();
  }

  const std::shared_ptr<airmap::Context>& context() const {
    return context_;
  }

 private:
  std::shared_ptr<airmap::Context> context_;
  std::thread worker_;
};

}  // namespace

struct airmap::services::Client::Private {
  explicit Private(const Client::Configuration& configuration, const std::shared_ptr<ContextRunner>& context_runner,
                   const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client)
      : configuration_{configuration},
        context_runner_{context_runner},
        dispatcher_{dispatcher},
        client_{client},
        advisory_{airmap::services::Advisory::create(dispatcher_, client_)},
        aircrafts_{airmap::services::Aircrafts::create(dispatcher_, client_)},
        airspaces_{airmap::services::Airspaces::create(dispatcher_, client_)},
        authenticator_{airmap::services::Authenticator::create(dispatcher_, client_)},
        flight_plans_{airmap::services::FlightPlans::create(dispatcher_, client_)},
        flights_{airmap::services::Flights::create(dispatcher_, client_)},
        pilots_{airmap::services::Pilots::create(dispatcher_, client_)},
        rulesets_{airmap::services::RuleSets::create(dispatcher_, client_)},
        status_{airmap::services::Status::create(dispatcher_, client_)},
        telemetry_{airmap::services::Telemetry::create(dispatcher_, client_)},
        traffic_{airmap::services::Traffic::create(dispatcher_, client_)} {
  }

  ~Private() {
    context_runner_->stop();
  }

  Client::Configuration configuration_;
  std::shared_ptr<ContextRunner> context_runner_;
  std::shared_ptr<airmap::services::Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
  std::shared_ptr<airmap::services::Advisory> advisory_;
  std::shared_ptr<airmap::services::Aircrafts> aircrafts_;
  std::shared_ptr<airmap::services::Airspaces> airspaces_;
  std::shared_ptr<airmap::services::Authenticator> authenticator_;
  std::shared_ptr<airmap::services::FlightPlans> flight_plans_;
  std::shared_ptr<airmap::services::Flights> flights_;
  std::shared_ptr<airmap::services::Pilots> pilots_;
  std::shared_ptr<airmap::services::RuleSets> rulesets_;
  std::shared_ptr<airmap::services::Status> status_;
  std::shared_ptr<airmap::services::Telemetry> telemetry_;
  std::shared_ptr<airmap::services::Traffic> traffic_;
};

void airmap::services::Client::create(const Client::Configuration& configuration, const std::shared_ptr<Logger>& logger,
                                QObject* parent, const CreateCallback& cb) {
  register_types();

  auto result     = Context::create(logger);
  auto dispatcher = std::make_shared<Dispatcher>(result.value());

  if (!result) {
    dispatcher->dispatch_to_qt([result, cb]() { cb(CreateResult{result.error()}); });
  } else {
    auto cr = std::make_shared<ContextRunner>(result.value());

    cr->context()->create_client_with_configuration(
        configuration, [dispatcher, configuration, parent, cr, cb](const auto& result) {
          dispatcher->dispatch_to_qt([dispatcher, configuration, parent, cr, result, cb]() {
            if (result) {
              cb(CreateResult{new Client{
                  std::unique_ptr<Private>{new Private{configuration, cr, dispatcher, result.value()}}, parent}});
            } else {
              cb(CreateResult{result.error()});
            }
          });
        });

    cr->start();
  }
}

airmap::services::Client::Client(std::unique_ptr<Private>&& d, QObject* parent) : QObject{parent}, d_{std::move(d)} {
}

airmap::services::Client::~Client() = default;

// From airmap::Client
airmap::Authenticator& airmap::services::Client::authenticator() {
  return *d_->authenticator_;
}

airmap::Advisory& airmap::services::Client::advisory() {
  return *d_->advisory_;
}

airmap::Aircrafts& airmap::services::Client::aircrafts() {
  return *d_->aircrafts_;
}

airmap::Airspaces& airmap::services::Client::airspaces() {
  return *d_->airspaces_;
}

airmap::FlightPlans& airmap::services::Client::flight_plans() {
  return *d_->flight_plans_;
}

airmap::Flights& airmap::services::Client::flights() {
  return *d_->flights_;
}

airmap::Pilots& airmap::services::Client::pilots() {
  return *d_->pilots_;
}

airmap::RuleSets& airmap::services::Client::rulesets() {
  return *d_->rulesets_;
}

airmap::Status& airmap::services::Client::status() {
  return *d_->status_;
}

airmap::Telemetry& airmap::services::Client::telemetry() {
  return *d_->telemetry_;
}

airmap::Traffic& airmap::services::Client::traffic() {
  return *d_->traffic_;
}
