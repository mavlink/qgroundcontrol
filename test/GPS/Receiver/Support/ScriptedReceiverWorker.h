#pragma once

#include <memory>
#include <optional>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSReceiver.h"
#include "GPSReceiverWorker.h"
#include "Support/GPSTestHelpers.h"

namespace GPSTest {

class ScriptedReceiverWorker : public GPSReceiverWorker
{
    Q_OBJECT

public:
    ScriptedReceiverWorker(GPSReceiverWorker::TransportFactory transportFactory, GPSType type,
                           const GPSReceiverConfig& config, QObject* parent = nullptr)
        : GPSReceiverWorker(std::move(transportFactory), type, config, parent)
        , _type(type)
        , _capturedConfig(config)
    {}

    void start() override {}

    void stop() override { _stopped = true; }

    GPSType type() const { return _type; }

    const GPSReceiverConfig& capturedConfig() const { return _capturedConfig; }

    bool stopped() const { return _stopped; }

    void detected(GPSType type)
    {
        emit receiverDetected(type);
        deliverQueuedCalls();
    }

    /// Reports what the detection of an Automatic session looks for with the link at @a baud: @a family, where the
    /// session's hint says it was when @a remembered and else in its turn, or the output of any family
    /// (GPSType::Automatic).
    void looksFor(GPSType family, unsigned baud, bool remembered = false)
    {
        emit detecting(family, baud, remembered);
        deliverQueuedCalls();
    }

    /// Reports the receiver ready, and @a detected as what an Automatic session found.
    void ready(const QString& identity = {}, const GPSDetectionHint& detected = {})
    {
        emit receiverReady(identity, detected);
        deliverQueuedCalls();
    }

    void position(const GPSPositionReport& report)
    {
        emit positionUpdated(report);
        deliverQueuedCalls();
    }

    void satellites(const GPSSatelliteReport& report)
    {
        emit satelliteInfoUpdated(report);
        deliverQueuedCalls();
    }

    void baseStatus(const GPSBaseStatus& status)
    {
        emit baseStatusUpdated(status);
        deliverQueuedCalls();
    }

    void rtcm(const QByteArray& bytes, qint64 receivedAtMs = 0)
    {
        emit rtcmDataReceived(bytes, receivedAtMs);
        deliverQueuedCalls();
    }

    void input(GPSInputProblem problem)
    {
        emit inputProblem(problem);
        deliverQueuedCalls();
    }

    void fail(GPSConnectionError error, const QString& detail = {})
    {
        emit connectionError(error, detail);
        deliverQueuedCalls();
    }

    void finish()
    {
        emit finished();
        deliverQueuedCalls();
    }

private:
    GPSType _type;
    GPSReceiverConfig _capturedConfig;
    bool _stopped = false;
};

/// Creates the scripted workers of a receiver's sessions and keeps track of them. A receiver that ends a session
/// leaves its worker to finish by itself, which a scripted worker does only when its test says so: those that are
/// left are deleted once the factory and the receivers it was given to are gone, so a test need not shut its
/// receiver down for them.
class ScriptedReceiverWorkerFactory
{
public:
    GPSReceiver::WorkerFactory workerFactory()
    {
        return [workers = _workers](GPSReceiverWorker::TransportFactory transportFactory, GPSType type,
                                    const GPSReceiverConfig& config, QObject* parent) {
            auto* worker = new ScriptedReceiverWorker(std::move(transportFactory), type, config, parent);
            workers->created.append(worker);
            return worker;
        };
    }

    qsizetype count() const { return _workers->created.size(); }

    ScriptedReceiverWorker* current() const
    {
        return _workers->created.isEmpty() ? nullptr : _workers->created.last().data();
    }

    /// Completes the cancellation of retired workers, which hold their serial port until then.
    void finishRetired() const
    {
        for (const QPointer<ScriptedReceiverWorker>& retired : _workers->created) {
            if (retired && retired->stopped()) {
                retired->finish();
            }
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

private:
    /// The workers created, which the factory shares with every workerFactory() it gave out. A receiver keeps its
    /// copy until it is destroyed, when it has retired its last session, so these outlive every session whose
    /// worker they list, whether the receiver or the factory is destroyed first.
    struct Workers
    {
        Workers() = default;
        Workers(const Workers&) = delete;
        Workers& operator=(const Workers&) = delete;

        ~Workers()
        {
            // A retired worker has no owner. One that still has a session is deleted with it.
            for (const QPointer<ScriptedReceiverWorker>& worker : std::as_const(created)) {
                if (worker && !worker->parent()) {
                    delete worker.data();
                }
            }
        }

        QList<QPointer<ScriptedReceiverWorker>> created;
    };

    const std::shared_ptr<Workers> _workers = std::make_shared<Workers>();
};

}  // namespace GPSTest
