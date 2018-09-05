#ifndef AIRMAP_QT_CLIENT_H_
#define AIRMAP_QT_CLIENT_H_

#include <airmap/client.h>
#include <airmap/context.h>
#include <airmap/error.h>
#include <airmap/logger.h>
#include <airmap/outcome.h>

#include <QObject>

namespace airmap {
/// @namespace namespace qt bundles up types and functions that help with integrating AirMap functionality
/// into Qt-based applications and libraries.
namespace qt {

/// Client implements the airmap::Client interface, bridging over between
/// the Qt event loop and the native event loop of the native airmap::Client.
///
/// All callback invocations that might happen in the context of a Client instance
/// are dispatched to the Qt applications' main thread.
class Client : public QObject, public airmap::Client {
 public:
  using CreateResult   = Outcome<Client*, Error>;
  using CreateCallback = std::function<void(const CreateResult&)>;

  /// create creates a new Client instance with parent 'parent', logging to 'logger', using the config
  /// 'configuration'. The result of the request is reported to 'cb', on the thread that issued the create request.
  ///
  /// Please note that this function must be called on Qt's main thread as event dispatching between different
  /// event loops to the Qt world is set up here.
  static void create(const Client::Configuration& configuration, const std::shared_ptr<Logger>& logger, QObject* parent,
                     const CreateCallback& cb);

  ~Client();

  // From airmap::Client
  Authenticator& authenticator() override;
  Advisory& advisory() override;
  Aircrafts& aircrafts() override;
  Airspaces& airspaces() override;
  FlightPlans& flight_plans() override;
  Flights& flights() override;
  Pilots& pilots() override;
  RuleSets& rulesets() override;
  Status& status() override;
  Telemetry& telemetry() override;
  Traffic& traffic() override;

 private:
  /// @cond
  struct Private;
  Client(std::unique_ptr<Private>&& d, QObject* parent);
  std::unique_ptr<Private> d_;
  /// @endcond
};

}  // namespace qt
}  // namespace airmap

/// @example qt/client.cpp
/// Illustrates how to use airmap::qt::Client, airmap::qt::DispatchingLogger and airmap::qt::Logger.

#endif  // AIRMAP_QT_CLIENT_H_
