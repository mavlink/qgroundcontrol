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
#ifndef AIRMAP_QT_LOGGER_H_
#define AIRMAP_QT_LOGGER_H_

#include <airmap/logger.h>
#include <airmap/visibility.h>

#include <QLoggingCategory>

#include <memory>

namespace airmap {
namespace qt {

/// Logger is an airmap::Logger implementation that uses to
/// Qt's logging facilities.
class AIRMAP_EXPORT Logger : public airmap::Logger {
 public:
  /// logging_category returns a QLoggingCategory instance
  /// that enables calling code to fine-tune logging behavior of a Logger instance.
  QLoggingCategory& logging_category();

  /// Logger initializes a new instance.
  Logger();
  /// ~Logger cleans up all resources held by a Logger instance.
  ~Logger();

  // From airmap::Logger
  void log(Severity severity, const char* message, const char* component) override;
  bool should_log(Severity severity, const char* message, const char* component) override;

 private:
  struct Private;
  std::unique_ptr<Private> d_;
};

/// DispatchingLogger is an airmap::Logger implementation that dispatches to Qt's main
/// event loop for logger invocation
class AIRMAP_EXPORT DispatchingLogger : public airmap::Logger {
 public:
  /// DispatchingLogger initializes a new instance with 'next'.
  DispatchingLogger(const std::shared_ptr<airmap::Logger>& next);
  /// ~DispatchingLogging cleans up all resources held a DispatchingLogger instance.
  ~DispatchingLogger();

  // From airmap::Logger
  void log(Severity severity, const char* message, const char* component) override;
  bool should_log(Severity severity, const char* message, const char* component) override;

 private:
  struct Private;
  std::unique_ptr<Private> d_;
};

}  // namespace qt
}  // namespace airmap

#endif  // AIRMAP_QT_LOGGER_H_
