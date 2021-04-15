<<<<<<< HEAD
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
=======
#include <Airmap/qt/client.h>

#include <Airmap/qt/dispatcher.h>

#include <Airmap/qt/advisory.h>
#include <Airmap/qt/aircrafts.h>
#include <Airmap/qt/airspaces.h>
#include <Airmap/qt/authenticator.h>
#include <Airmap/qt/flight_plans.h>
#include <Airmap/qt/flights.h>
#include <Airmap/qt/pilots.h>
#include <Airmap/qt/rulesets.h>
#include <Airmap/qt/status.h>
#include <Airmap/qt/telemetry.h>
#include <Airmap/qt/traffic.h>
#include <Airmap/qt/types.h>
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes

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

<<<<<<< HEAD
struct airmap::services::Client::Private {
=======
struct airmap::qt::Client::Private {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  explicit Private(const Client::Configuration& configuration, const std::shared_ptr<ContextRunner>& context_runner,
                   const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client)
      : configuration_{configuration},
        context_runner_{context_runner},
        dispatcher_{dispatcher},
        client_{client},
<<<<<<< HEAD
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
=======
        advisory_{airmap::qt::Advisory::create(dispatcher_, client_)},
        aircrafts_{airmap::qt::Aircrafts::create(dispatcher_, client_)},
        airspaces_{airmap::qt::Airspaces::create(dispatcher_, client_)},
        authenticator_{airmap::qt::Authenticator::create(dispatcher_, client_)},
        flight_plans_{airmap::qt::FlightPlans::create(dispatcher_, client_)},
        flights_{airmap::qt::Flights::create(dispatcher_, client_)},
        pilots_{airmap::qt::Pilots::create(dispatcher_, client_)},
        rulesets_{airmap::qt::RuleSets::create(dispatcher_, client_)},
        status_{airmap::qt::Status::create(dispatcher_, client_)},
        telemetry_{airmap::qt::Telemetry::create(dispatcher_, client_)},
        traffic_{airmap::qt::Traffic::create(dispatcher_, client_)} {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  }

  ~Private() {
    context_runner_->stop();
  }

  Client::Configuration configuration_;
  std::shared_ptr<ContextRunner> context_runner_;
<<<<<<< HEAD
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
=======
  std::shared_ptr<airmap::qt::Dispatcher> dispatcher_;
  std::shared_ptr<airmap::Client> client_;
  std::shared_ptr<airmap::qt::Advisory> advisory_;
  std::shared_ptr<airmap::qt::Aircrafts> aircrafts_;
  std::shared_ptr<airmap::qt::Airspaces> airspaces_;
  std::shared_ptr<airmap::qt::Authenticator> authenticator_;
  std::shared_ptr<airmap::qt::FlightPlans> flight_plans_;
  std::shared_ptr<airmap::qt::Flights> flights_;
  std::shared_ptr<airmap::qt::Pilots> pilots_;
  std::shared_ptr<airmap::qt::RuleSets> rulesets_;
  std::shared_ptr<airmap::qt::Status> status_;
  std::shared_ptr<airmap::qt::Telemetry> telemetry_;
  std::shared_ptr<airmap::qt::Traffic> traffic_;
};

void airmap::qt::Client::create(const Client::Configuration& configuration, const std::shared_ptr<Logger>& logger,
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
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

<<<<<<< HEAD
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
=======
airmap::qt::Client::Client(std::unique_ptr<Private>&& d, QObject* parent) : QObject{parent}, d_{std::move(d)} {
}

airmap::qt::Client::~Client() = default;

// From airmap::Client
airmap::Authenticator& airmap::qt::Client::authenticator() {
  return *d_->authenticator_;
}

airmap::Advisory& airmap::qt::Client::advisory() {
  return *d_->advisory_;
}

airmap::Aircrafts& airmap::qt::Client::aircrafts() {
  return *d_->aircrafts_;
}

airmap::Airspaces& airmap::qt::Client::airspaces() {
  return *d_->airspaces_;
}

airmap::FlightPlans& airmap::qt::Client::flight_plans() {
  return *d_->flight_plans_;
}

airmap::Flights& airmap::qt::Client::flights() {
  return *d_->flights_;
}

airmap::Pilots& airmap::qt::Client::pilots() {
  return *d_->pilots_;
}

airmap::RuleSets& airmap::qt::Client::rulesets() {
  return *d_->rulesets_;
}

airmap::Status& airmap::qt::Client::status() {
  return *d_->status_;
}

airmap::Telemetry& airmap::qt::Client::telemetry() {
  return *d_->telemetry_;
}

airmap::Traffic& airmap::qt::Client::traffic() {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  return *d_->traffic_;
}
