#pragma once

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

    void ready(const QString& identity = {})
    {
        emit receiverReady(identity);
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

    void survey(const GPSSurveyReport& report)
    {
        emit surveyInStatusUpdated(report);
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

class ScriptedReceiverWorkerFactory
{
public:
    GPSReceiver::WorkerFactory workerFactory()
    {
        return [this](GPSReceiverWorker::TransportFactory transportFactory, GPSType type,
                      const GPSReceiverConfig& config, QObject* parent) {
            auto* worker = new ScriptedReceiverWorker(std::move(transportFactory), type, config, parent);
            _workers.append(worker);
            return worker;
        };
    }

    qsizetype count() const { return _workers.size(); }

    ScriptedReceiverWorker* current() const { return _workers.isEmpty() ? nullptr : _workers.last().data(); }

    /// Completes the cancellation of retired workers, which hold their serial port until then.
    void finishRetired() const
    {
        for (const QPointer<ScriptedReceiverWorker>& retired : _workers) {
            if (retired && retired->stopped()) {
                retired->finish();
            }
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

private:
    QList<QPointer<ScriptedReceiverWorker>> _workers;
};

}  // namespace GPSTest
