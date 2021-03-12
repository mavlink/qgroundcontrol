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
#ifndef AIRMAP_MONITOR_CLIENT_H_
#define AIRMAP_MONITOR_CLIENT_H_

#include <airmap/do_not_copy_or_move.h>
#include <airmap/error.h>
#include <airmap/traffic.h>
#include <airmap/visibility.h>

#include <memory>
#include <vector>

namespace airmap {
/// namespace monitor bundles up types and functions to
/// interact with the AirMap monitor daemon.
namespace monitor {

/// Client provides access to the AirMap monitor service.
class AIRMAP_EXPORT Client : DoNotCopyOrMove {
 public:
  /// Configuration bundles up creation-time parameters of a Client.
  struct AIRMAP_EXPORT Configuration {
    std::string endpoint;            ///< The remote endpoint hosting the service.
    std::shared_ptr<Logger> logger;  ///< The logger instance.
  };

  /// Updates models updates delivered to clients.
  struct AIRMAP_EXPORT Update {
    std::vector<Traffic::Update> traffic;  ///< Traffic updates.
  };

  /// UpdateStream abstracts a source of incoming updates.
  class AIRMAP_EXPORT UpdateStream : DoNotCopyOrMove {
   public:
    /// Reveiver models an entity interested in receiving updates.
    class AIRMAP_EXPORT Receiver : DoNotCopyOrMove {
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
  struct AIRMAP_EXPORT ConnectToUpdates {
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
