<<<<<<< HEAD
#include <Airmap/services/logger.h>

#include <Airmap/services/dispatcher.h>

struct airmap::services::Logger::Private {
  QLoggingCategory logging_category{"airmap"};
};

struct airmap::services::DispatchingLogger::Private {
=======
#include <Airmap/qt/logger.h>

#include <Airmap/qt/dispatcher.h>

struct airmap::qt::Logger::Private {
  QLoggingCategory logging_category{"airmap"};
};

struct airmap::qt::DispatchingLogger::Private {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  std::shared_ptr<airmap::Logger> next;
  std::shared_ptr<Dispatcher::ToQt> dispatcher;
};

<<<<<<< HEAD
airmap::services::Logger::Logger() : d_{new Private{}} {
}
airmap::services::Logger::~Logger() {
}

QLoggingCategory& airmap::services::Logger::logging_category() {
=======
airmap::qt::Logger::Logger() : d_{new Private{}} {
}
airmap::qt::Logger::~Logger() {
}

QLoggingCategory& airmap::qt::Logger::logging_category() {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  static QLoggingCategory lc{"airmap"};
  return lc;
}

<<<<<<< HEAD
void airmap::services::Logger::log(Severity severity, const char* message, const char*) {
=======
void airmap::qt::Logger::log(Severity severity, const char* message, const char*) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  switch (severity) {
    case Severity::debug:
      qCDebug(logging_category(), "%s", message);
      break;
    case Severity::info:
      qCInfo(logging_category(), "%s", message);
      break;
    case Severity::error:
      qCWarning(logging_category(), "%s", message);
      break;
    default:
      break;
  }
}

<<<<<<< HEAD
bool airmap::services::Logger::should_log(Severity severity, const char*, const char*) {
=======
bool airmap::qt::Logger::should_log(Severity severity, const char*, const char*) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  switch (severity) {
    case Severity::debug:
      return logging_category().isDebugEnabled();
    case Severity::info:
      return logging_category().isInfoEnabled();
    case Severity::error:
      return logging_category().isWarningEnabled();
    default:
      break;
  }

  return true;
}

<<<<<<< HEAD
airmap::services::DispatchingLogger::DispatchingLogger(const std::shared_ptr<airmap::Logger>& next)
    : d_{new Private{next, Dispatcher::ToQt::create()}} {
}

airmap::services::DispatchingLogger::~DispatchingLogger() {
}

void airmap::services::DispatchingLogger::log(Severity severity, const char* message, const char* component) {
=======
airmap::qt::DispatchingLogger::DispatchingLogger(const std::shared_ptr<airmap::Logger>& next)
    : d_{new Private{next, Dispatcher::ToQt::create()}} {
}

airmap::qt::DispatchingLogger::~DispatchingLogger() {
}

void airmap::qt::DispatchingLogger::log(Severity severity, const char* message, const char* component) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  std::string cmessage{message};
  std::string ccomponent{component};
  auto cnext{d_->next};

  d_->dispatcher->dispatch([severity, cmessage, ccomponent, cnext] {
    if (cnext->should_log(severity, cmessage.c_str(), ccomponent.c_str()))
      cnext->log(severity, cmessage.c_str(), ccomponent.c_str());
  });
}

<<<<<<< HEAD
bool airmap::services::DispatchingLogger::should_log(Severity, const char*, const char*) {
=======
bool airmap::qt::DispatchingLogger::should_log(Severity, const char*, const char*) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  // We have to accept all incoming log messages and postpone
  // the actual evaluation of should_log in the context of next until we
  // run on the correct thread.
  return true;
}
