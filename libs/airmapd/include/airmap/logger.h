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
#ifndef AIRMAP_LOGGER_H_
#define AIRMAP_LOGGER_H_

#include <airmap/do_not_copy_or_move.h>
#include <airmap/visibility.h>

#include <iostream>
#include <memory>

namespace airmap {

/// Logger abstracts logging of human-readable message
/// providing details on the operation of the system.
class AIRMAP_EXPORT Logger : DoNotCopyOrMove {
 public:
  /// Severity enumerates all known levels of severity
  enum class Severity { debug = 0, info = 1, error = 2 };

  /// debug logs a message from component with Severity::debug.
  void debug(const char* message, const char* component);

  /// info logs a message from component with Severity::info.
  void info(const char* message, const char* component);

  /// error logs a message from component with Severity::error.
  void error(const char* message, const char* component);

  /// log handles the incoming log message originating from component.
  /// Implementation should handle the case of component being a nullptr
  /// gracefully.
  virtual void log(Severity severity, const char* message, const char* component) = 0;

  /// should_log should return true if 'message' with 'severity' originating from
  /// 'component' should be logged.
  ///
  /// Implementations should handle the case of either message or component being nullptr
  /// gracefully.
  virtual bool should_log(Severity severity, const char* message, const char* component) = 0;

 protected:
  Logger() = default;
};

/// operator< returns true iff the numeric value of lhs < rhs.
AIRMAP_EXPORT bool operator<(Logger::Severity lhs, Logger::Severity rhs);

/// operator>> parses severity from in.
AIRMAP_EXPORT std::istream& operator>>(std::istream& in, Logger::Severity& severity);

/// create_default_logger returns a Logger implementation writing
/// log messages to 'out'.
AIRMAP_EXPORT std::shared_ptr<Logger> create_default_logger(std::ostream& out = std::cerr);

/// create_filtering_logger returns a logger that filters out log entries
/// with a severity smaller than the configurated severity.
AIRMAP_EXPORT std::shared_ptr<Logger> create_filtering_logger(Logger::Severity severity,
                                                              const std::shared_ptr<Logger>& logger);

/// create_null_logger returns a logger that does the equivalent of
/// > /dev/null.
AIRMAP_EXPORT std::shared_ptr<Logger> create_null_logger();

}  // namespace airmap

#endif  // AIRMAP_LOGGER_H_
