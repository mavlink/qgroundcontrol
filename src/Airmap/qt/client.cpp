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

struct airmap::qt::Client::Private {
  explicit Private(const Client::Configuration& configuration, const std::shared_ptr<ContextRunner>& context_runner,
                   const std::shared_ptr<Dispatcher>& dispatcher, const std::shared_ptr<airmap::Client>& client)
      : configuration_{configuration},
        context_runner_{context_runner},
        dispatcher_{dispatcher},
        client_{client},
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
  }

  ~Private() {
    context_runner_->stop();
  }

  Client::Configuration configuration_;
  std::shared_ptr<ContextRunner> context_runner_;
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
  return *d_->traffic_;
}
