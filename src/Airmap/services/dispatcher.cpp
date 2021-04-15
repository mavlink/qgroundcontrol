<<<<<<< HEAD
#include <Airmap/services/dispatcher.h>
=======
#include <Airmap/qt/dispatcher.h>
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes

#include <QCoreApplication>
#include <QThread>

#include <cassert>

<<<<<<< HEAD
QEvent::Type airmap::services::Dispatcher::Event::registered_type() {
=======
QEvent::Type airmap::qt::Dispatcher::Event::registered_type() {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  static const Type rt = static_cast<Type>(registerEventType());
  return rt;
}

<<<<<<< HEAD
airmap::services::Dispatcher::Event::Event(const std::function<void()>& task) : QEvent{registered_type()}, task_{task} {
}

void airmap::services::Dispatcher::Event::dispatch() {
  task_();
}

std::shared_ptr<airmap::services::Dispatcher::ToQt> airmap::services::Dispatcher::ToQt::create() {
  return std::shared_ptr<ToQt>{new ToQt{}};
}

airmap::services::Dispatcher::ToQt::ToQt() {
}

void airmap::services::Dispatcher::ToQt::dispatch(const Task& task) {
=======
airmap::qt::Dispatcher::Event::Event(const std::function<void()>& task) : QEvent{registered_type()}, task_{task} {
}

void airmap::qt::Dispatcher::Event::dispatch() {
  task_();
}

std::shared_ptr<airmap::qt::Dispatcher::ToQt> airmap::qt::Dispatcher::ToQt::create() {
  return std::shared_ptr<ToQt>{new ToQt{}};
}

airmap::qt::Dispatcher::ToQt::ToQt() {
}

void airmap::qt::Dispatcher::ToQt::dispatch(const Task& task) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  auto sp = shared_from_this();

  QCoreApplication::postEvent(this, new Event{[sp, task]() { task(); }});
}

<<<<<<< HEAD
bool airmap::services::Dispatcher::ToQt::event(QEvent* event) {
=======
bool airmap::qt::Dispatcher::ToQt::event(QEvent* event) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  assert(QCoreApplication::instance());
  assert(QThread::currentThread() == QCoreApplication::instance()->thread());

  if (event->type() == Event::registered_type()) {
    event->accept();

    if (auto e = dynamic_cast<Event*>(event)) {
      e->dispatch();
    }

    return true;
  }

  return false;
}

<<<<<<< HEAD
airmap::services::Dispatcher::Dispatcher(const std::shared_ptr<Context>& context)
    : to_qt_{ToQt::create()}, context_{context} {
}

void airmap::services::Dispatcher::dispatch_to_qt(const std::function<void()>& task) {
  to_qt_->dispatch(task);
}

void airmap::services::Dispatcher::dispatch_to_airmap(const std::function<void()>& task) {
=======
airmap::qt::Dispatcher::Dispatcher(const std::shared_ptr<Context>& context)
    : to_qt_{ToQt::create()}, context_{context} {
}

void airmap::qt::Dispatcher::dispatch_to_qt(const std::function<void()>& task) {
  to_qt_->dispatch(task);
}

void airmap::qt::Dispatcher::dispatch_to_airmap(const std::function<void()>& task) {
>>>>>>> renamed qt dir to services for all of the platform-sdk service interface classes
  context_->schedule_in(task);
}

// From QObject
