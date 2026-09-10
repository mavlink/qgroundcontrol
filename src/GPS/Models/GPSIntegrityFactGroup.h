#pragma once

#include "FactGroup.h"
#include "GPSIntegrityObservation.h"
#include "GPSIntegrityStore.h"

class GPSIntegrityFactGroup : public FactGroup
{
    Q_OBJECT
    Q_PROPERTY(Fact* systemErrors READ systemErrors CONSTANT)
    Q_PROPERTY(Fact* spoofingState READ spoofingState CONSTANT)
    Q_PROPERTY(Fact* jammingState READ jammingState CONSTANT)
    Q_PROPERTY(Fact* authenticationState READ authenticationState CONSTANT)
    Q_PROPERTY(Fact* correctionsQuality READ correctionsQuality CONSTANT)
    Q_PROPERTY(Fact* systemQuality READ systemQuality CONSTANT)
    Q_PROPERTY(Fact* gnssSignalQuality READ gnssSignalQuality CONSTANT)
    Q_PROPERTY(Fact* postProcessingQuality READ postProcessingQuality CONSTANT)
    Q_PROPERTY(Fact* correctionsProtocol READ correctionsProtocol CONSTANT)
    Q_PROPERTY(Fact* correctionsUsed READ correctionsUsed CONSTANT)
    Q_PROPERTY(Fact* noisePerMillisecond READ noisePerMillisecond CONSTANT)
    Q_PROPERTY(Fact* automaticGainControl READ automaticGainControl CONSTANT)
    Q_PROPERTY(Fact* jammingIndicator READ jammingIndicator CONSTANT)
    Q_PROPERTY(Fact* correctionsCrcFailed READ correctionsCrcFailed CONSTANT)
    Q_PROPERTY(bool available READ available NOTIFY availabilityChanged)
    Q_PROPERTY(bool systemErrorsKnown READ systemErrorsKnown NOTIFY availabilityChanged)

public:
    explicit GPSIntegrityFactGroup(QObject* parent = nullptr, GPSRuntimeScheduler* scheduler = nullptr);
    ~GPSIntegrityFactGroup() override;

    Fact* systemErrors() { return &_systemErrors; }

    Fact* spoofingState() { return &_spoofingState; }

    Fact* jammingState() { return &_jammingState; }

    Fact* authenticationState() { return &_authenticationState; }

    Fact* correctionsQuality() { return &_correctionsQuality; }

    Fact* systemQuality() { return &_systemQuality; }

    Fact* gnssSignalQuality() { return &_gnssSignalQuality; }

    Fact* postProcessingQuality() { return &_postProcessingQuality; }

    Fact* correctionsProtocol() { return &_correctionsProtocol; }

    Fact* correctionsUsed() { return &_correctionsUsed; }

    Fact* noisePerMillisecond() { return &_noisePerMillisecond; }

    Fact* automaticGainControl() { return &_automaticGainControl; }

    Fact* jammingIndicator() { return &_jammingIndicator; }

    Fact* correctionsCrcFailed() { return &_correctionsCrcFailed; }

    bool available() const { return _available; }

    bool systemErrorsKnown() const { return _systemErrorsKnown; }

    void update(const GPSIntegrityObservation& observation);
    void reset();

signals:
    void availabilityChanged();

private:
    void _refresh();
    GPSIntegrityStore _store;
    Fact _systemErrors = Fact(0, QStringLiteral("systemErrors"), FactMetaData::valueTypeUint32);
    Fact _spoofingState = Fact(0, QStringLiteral("spoofingState"), FactMetaData::valueTypeUint8);
    Fact _jammingState = Fact(0, QStringLiteral("jammingState"), FactMetaData::valueTypeUint8);
    Fact _authenticationState = Fact(0, QStringLiteral("authenticationState"), FactMetaData::valueTypeUint8);
    Fact _correctionsQuality = Fact(0, QStringLiteral("correctionsQuality"), FactMetaData::valueTypeUint8);
    Fact _systemQuality = Fact(0, QStringLiteral("systemQuality"), FactMetaData::valueTypeUint8);
    Fact _gnssSignalQuality = Fact(0, QStringLiteral("gnssSignalQuality"), FactMetaData::valueTypeUint8);
    Fact _postProcessingQuality = Fact(0, QStringLiteral("postProcessingQuality"), FactMetaData::valueTypeUint8);
    Fact _correctionsProtocol = Fact(0, QStringLiteral("correctionsProtocol"), FactMetaData::valueTypeUint8);
    Fact _correctionsUsed = Fact(0, QStringLiteral("correctionsUsed"), FactMetaData::valueTypeUint8);
    Fact _noisePerMillisecond = Fact(0, QStringLiteral("noisePerMillisecond"), FactMetaData::valueTypeInt32);
    Fact _automaticGainControl = Fact(0, QStringLiteral("automaticGainControl"), FactMetaData::valueTypeInt32);
    Fact _jammingIndicator = Fact(0, QStringLiteral("jammingIndicator"), FactMetaData::valueTypeInt32);
    Fact _correctionsCrcFailed = Fact(0, QStringLiteral("correctionsCrcFailed"), FactMetaData::valueTypeInt32);
    quint64 _revision = 0;
    bool _available = false;
    bool _systemErrorsKnown = false;
};
