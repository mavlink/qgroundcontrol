#ifndef AIRMAP_TRAFFIC_H_
#define AIRMAP_TRAFFIC_H_

#include <airmap/date_time.h>
#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/logger.h>
#include <airmap/outcome.h>

#include <functional>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

namespace airmap {

/// Traffic provides access to the AirMap situational awareness
/// and traffic alerts.
class Traffic : DoNotCopyOrMove {
 public:
  /// Update bundles together information about aerial traffic
  /// relevant to a UAV flight.
  struct Update {
    /// Type enumerates all known types of Traffic::Update.
    enum class Type {
      unknown,                ///< Marks the unknown type.
      situational_awareness,  ///< Marks updates that provide planning information to operators and vehicles.
      alert                   ///< Marks updates about aircrafts that are likely to collide with the current aircraft.
    };

    std::string id;           ///< The unique id of the underlying track in the context of AirMap.
    std::string aircraft_id;  ///< The 'other' aircraft's id.
    double latitude;          ///< The latitude of the other aircraft in [°].
    double longitude;         ///< The longitude of the other aircraft in [°].
    double altitude;          ///< The altitude of the other aircraft in [m].
    double ground_speed;      ///< The speed over ground of the other aircraft in [m/s].
    double heading;           ///< The heading of the other aircraft in [°].
    double direction;         ///< The direction of the other aircraft in relation to the current aircraft in [°].
    DateTime recorded;        ///< The time when the datum triggering the udpate was recorded.
    DateTime timestamp;       ///< The time when the update was generated.
  };

  /// Monitor models handling of individual subscribers
  /// to per-flight alerts and awareness notices.
  class Monitor : DoNotCopyOrMove {
   public:
    /// Parameters bundles up input parameters.
    struct Params {
      std::string flight_id;      ///< The id of the flight for which traffic udpates should be started.
      std::string authorization;  ///< The authorization token.
    };

    /// Result models the outcome of calling Traffic::monitor.
    using Result = Outcome<std::shared_ptr<Monitor>, Error>;
    /// Callback describes the function signature of the callback that is
    /// invoked when a call to Traffic::monitor finishes.
    using Callback = std::function<void(const Result&)>;

    /// Subscriber abstracts handling of batches of Update instances.
    class Subscriber {
     public:
     virtual ~Subscriber() = default;
      /// handle_update is invoked when a new batch of Update instances
      /// is available.
      virtual void handle_update(Update::Type type, const std::vector<Update>& update) = 0;

     protected:
      Subscriber() = default;
    };

    /// FunctionalSubscriber is a convenience class that dispatches
    /// to a function 'f' for handling batches of Update instances.
    class FunctionalSubscriber : public Subscriber {
     public:
      /// FunctionalSubscriber initializes a new instance with 'f'.
      explicit FunctionalSubscriber(const std::function<void(Update::Type, const std::vector<Update>&)>& f);
      // From subscriber
      void handle_update(Update::Type type, const std::vector<Update>& update) override;

     private:
      std::function<void(Update::Type, const std::vector<Update>&)> f_;
    };

    /// LoggingSubscriber is a convenience class that logs incoming batches
    /// of Update instances.
    class LoggingSubscriber : public Subscriber {
     public:
      /// LoggingSubscriber initializes an instance with 'component', feeding
      /// log entries to 'logger'. Please note that no change of ownership takes
      /// place for 'component' and the lifetime of component has to exceed the
      /// lifetime of a LoggingSubscriber instance.
      explicit LoggingSubscriber(const char* component, const std::shared_ptr<Logger>& logger);

      // From Subscriber
      void handle_update(Update::Type type, const std::vector<Update>& update) override;

     private:
      const char* component_;
      std::shared_ptr<Logger> logger_;
    };

    /// subscribe registers 'subscriber' such that subsequent batches of
    /// Update instances are delivered to 'subscriber'.
    virtual void subscribe(const std::shared_ptr<Subscriber>& subscriber) = 0;

    /// unsubscribe unregisters 'subscriber'.
    virtual void unsubscribe(const std::shared_ptr<Subscriber>& subscriber) = 0;

   protected:
    Monitor() = default;
  };

  /// monitor subscribes the user and flight described in 'params' to
  /// the AirMap traffic services and reports the result to 'cb'.
  virtual void monitor(const Monitor::Params& params, const Monitor::Callback& cb) = 0;

 protected:
  /// @cond
  Traffic() = default;
  /// @endcond
};

/// operator<< inserts a textual representation of type into out.
std::ostream& operator<<(std::ostream& out, Traffic::Update::Type type);

}  // namespace airmap

#endif  // AIRMAP_TRAFFIC_H_
