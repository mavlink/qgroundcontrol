#pragma once

#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "GPSProvider.h"
#include "GPSRtk.h"

class ScriptedProvider : public GPSProvider
{
    Q_OBJECT

public:
    ScriptedProvider(GPSProvider::TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                     QObject* parent = nullptr)
        : GPSProvider(std::move(transportFactory), type, config, parent)
        , _type(type)
        , _capturedConfig(config)
    {}

    void start() override
    {
        _started = true;
        ++_startCount;
    }

    void stop() override
    {
        _stopped = true;
        ++_stopCount;
    }

    void setEndsWhenIdle(bool endsWhenIdle) override
    {
        GPSProvider::setEndsWhenIdle(endsWhenIdle);
        _endsWhenIdleSet = true;
        _capturedEndsWhenIdle = endsWhenIdle;
    }

    GPSType type() const { return _type; }

    const GPSReceiverConfig& capturedConfig() const { return _capturedConfig; }

    bool started() const { return _started; }

    bool stopped() const { return _stopped; }

    int startCount() const { return _startCount; }

    int stopCount() const { return _stopCount; }

    bool endsWhenIdleSet() const { return _endsWhenIdleSet; }

    bool capturedEndsWhenIdle() const { return _capturedEndsWhenIdle; }

    void ready(const QString& identity = {})
    {
        emit receiverReady(identity);
        deliverQueuedSignals();
    }

    void position(const GPSPositionReport& report)
    {
        emit positionUpdate(report);
        deliverQueuedSignals();
    }

    void satellites(const GPSSatelliteReport& report)
    {
        emit satelliteInfoUpdate(report);
        deliverQueuedSignals();
    }

    void survey(const GPSSurveyReport& report)
    {
        emit surveyInStatus(report);
        deliverQueuedSignals();
    }

    void rtcm(const QByteArray& bytes, qint64 receivedAtMs = 0)
    {
        emit RTCMDataUpdate(bytes, receivedAtMs);
        deliverQueuedSignals();
    }

    void fail(GPSConnectionError error, const QString& detail = {})
    {
        emit connectionError(error, detail);
        deliverQueuedSignals();
    }

    void finish()
    {
        emit finished();
        deliverQueuedSignals();
    }

    static void deliverQueuedSignals() { QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall); }

private:
    GPSType _type;
    GPSReceiverConfig _capturedConfig;
    bool _started = false;
    bool _stopped = false;
    bool _endsWhenIdleSet = false;
    bool _capturedEndsWhenIdle = true;
    int _startCount = 0;
    int _stopCount = 0;
};

class ScriptedProviderFactory
{
public:
    GPSRtk::ProviderFactory providerFactory()
    {
        return [this](GPSProvider::TransportFactory transportFactory, GPSType type, const GPSReceiverConfig& config,
                      QObject* parent) {
            auto* provider = new ScriptedProvider(std::move(transportFactory), type, config, parent);
            _providers.append(provider);
            return provider;
        };
    }

    qsizetype count() const { return _providers.size(); }

    ScriptedProvider* at(qsizetype index) const { return _providers.at(index); }

    ScriptedProvider* current() const { return _providers.isEmpty() ? nullptr : _providers.last().data(); }

    ScriptedProvider* previous() const
    {
        return _providers.size() < 2 ? nullptr : _providers.at(_providers.size() - 2).data();
    }

    QList<QPointer<ScriptedProvider>> providers() const { return _providers; }

private:
    QList<QPointer<ScriptedProvider>> _providers;
};
