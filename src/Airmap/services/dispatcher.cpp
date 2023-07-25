#include <Airmap/services/dispatcher.h>

#include <QCoreApplication>
#include <QThread>

#include <cassert>

QEvent::Type airmap::services::Dispatcher::Event::registered_type() {
  static const Type rt = static_cast<Type>(registerEventType());
  return rt;
}

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
  auto sp = shared_from_this();

  QCoreApplication::postEvent(this, new Event{[sp, task]() { task(); }});
}

bool airmap::services::Dispatcher::ToQt::event(QEvent* event) {
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

airmap::services::Dispatcher::Dispatcher(const std::shared_ptr<Context>& context)
    : to_qt_{ToQt::create()}, context_{context} {
}

void airmap::services::Dispatcher::dispatch_to_qt(const std::function<void()>& task) {
  to_qt_->dispatch(task);
}

void airmap::services::Dispatcher::dispatch_to_airmap(const std::function<void()>& task) {
  context_->schedule_in(task);
}

// From QObject
