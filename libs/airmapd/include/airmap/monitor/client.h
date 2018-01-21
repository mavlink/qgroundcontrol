#ifndef AIRMAP_MONITOR_CLIENT_H_
#define AIRMAP_MONITOR_CLIENT_H_

#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/traffic.h>

#include <memory>
#include <vector>

namespace airmap {
/// namespace monitor bundles up types and functions to
/// interact with the AirMap monitor daemon.
namespace monitor {

/// Client provides access to the AirMap monitor service.
class Client : DoNotCopyOrMove {
 public:
  /// Configuration bundles up creation-time parameters of a Client.
  struct Configuration {
    std::string endpoint;            ///< The remote endpoint hosting the service.
    std::shared_ptr<Logger> logger;  ///< The logger instance.
  };

  /// Updates models updates delivered to clients.
  struct Update {
    std::vector<Traffic::Update> traffic;  ///< Traffic updates.
  };

  /// UpdateStream abstracts a source of incoming updates.
  class UpdateStream : DoNotCopyOrMove {
   public:
    /// Reveiver models an entity interested in receiving updates.
    class Receiver : DoNotCopyOrMove {
     public:
      /// handle_update is invoked for every update sent out by the service.
      virtual void handle_update(const Update& update) = 0;
    };

    /// subscribe connects 'receiver' to the stream of updates.
    virtual void subscribe(const std::shared_ptr<Receiver>& receiver) = 0;

    /// unsubscribe disconnects 'receiver' from the stream of updates.
    virtual void unsubscribe(const std::shared_ptr<Receiver>& receiver) = 0;

   protected:
    UpdateStream() = default;
  };

  /// ConnectToUpdates bundles up types for calls to Client::connect_to_updates.
  struct ConnectToUpdates {
    /// Result models the outcome of calling Client::connect_to_updates.
    using Result = Outcome<std::shared_ptr<UpdateStream>, Error>;
    /// Callback models the async receiver for a call to Client::connect_to_updates.
    using Callback = std::function<void(const Result&)>;
  };

  /// connect_to_updates connects to incoming updates.
  virtual void connect_to_updates(const ConnectToUpdates::Callback& cb) = 0;

 protected:
  Client() = default;
};

}  // namespace monitor
}  // namespace airmap

/// @example monitor/client.cpp
/// Illustrates how to use airmap::monitor::Client to connect
/// to an AirMap monitor instance.

#endif  // AIRMAP_MONITOR_CLIENT_H_