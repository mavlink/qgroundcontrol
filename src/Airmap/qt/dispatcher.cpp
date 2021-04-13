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
#include <Airmap/qt/dispatcher.h>

#include <QCoreApplication>
#include <QThread>

#include <cassert>

QEvent::Type airmap::qt::Dispatcher::Event::registered_type() {
  static const Type rt = static_cast<Type>(registerEventType());
  return rt;
}

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
  auto sp = shared_from_this();

  QCoreApplication::postEvent(this, new Event{[sp, task]() { task(); }});
}

bool airmap::qt::Dispatcher::ToQt::event(QEvent* event) {
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

std::shared_ptr<airmap::qt::Dispatcher::ToNative> airmap::qt::Dispatcher::ToNative::create(
    const std::shared_ptr<Context>& context) {
  return std::shared_ptr<ToNative>{new ToNative{context}};
}

airmap::qt::Dispatcher::ToNative::ToNative(const std::shared_ptr<Context>& context) : context_{context} {
}

void airmap::qt::Dispatcher::ToNative::dispatch(const Task& task) {
  context_->schedule_in(task);
}

airmap::qt::Dispatcher::Dispatcher(const std::shared_ptr<Context>& context)
    : to_qt_{ToQt::create()}, to_native_{ToNative::create(context)} {
}

void airmap::qt::Dispatcher::dispatch_to_qt(const std::function<void()>& task) {
  to_qt_->dispatch(task);
}

void airmap::qt::Dispatcher::dispatch_to_airmap(const std::function<void()>& task) {
  to_native_->dispatch(task);
}

// From QObject
