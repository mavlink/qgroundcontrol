#pragma once

#include <utility>
#include <variant>

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QObject>

#include "GPSReceiverWorker.h"

namespace GPSTest {

/// Calls @a handler with each report of the kind @a Event that @a worker makes, on the thread that makes it, for as
/// long as @a context lives: the worker itself without one.
template <typename Event, typename Handler>
void onReport(GPSReceiverWorker& worker, Handler handler, const QObject* context = nullptr)
{
    (void) QObject::connect(
        &worker, &GPSReceiverWorker::reported, context ? context : &worker,
        [handler = std::move(handler)](const GPSReceiverEvent& event) mutable {
            if (const Event* const report = std::get_if<Event>(&event)) {
                handler(*report);
            }
        },
        Qt::DirectConnection);
}

/// The reports of the kind @a Event that a worker makes, recorded in order on the thread that makes them, as a
/// QSignalSpy records a signal.
template <typename Event>
class WorkerReports
{
public:
    explicit WorkerReports(GPSReceiverWorker& worker)
    {
        onReport<Event>(
            worker,
            [this](const Event& report) {
                const QMutexLocker locker(&_mutex);
                _reports.append(report);
            },
            &_recording);
    }

    qsizetype size() const
    {
        const QMutexLocker locker(&_mutex);
        return _reports.size();
    }

    bool isEmpty() const { return size() == 0; }

    Event at(qsizetype index) const
    {
        const QMutexLocker locker(&_mutex);
        return _reports.at(index);
    }

    Event first() const { return at(0); }

    Event last() const { return at(size() - 1); }

    QList<Event> all() const
    {
        const QMutexLocker locker(&_mutex);
        return _reports;
    }

private:
    mutable QMutex _mutex;
    QList<Event> _reports;
    /// Declared last, so that the recording ends before what it records into is destroyed.
    QObject _recording;
};

}  // namespace GPSTest
