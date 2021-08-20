#ifndef AIRMAP_QT_LOGGER_H_
#define AIRMAP_QT_LOGGER_H_

#include <airmap/logger.h>
#include <airmap/visibility.h>

#include <QLoggingCategory>

#include <memory>

namespace airmap {
namespace services {

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
