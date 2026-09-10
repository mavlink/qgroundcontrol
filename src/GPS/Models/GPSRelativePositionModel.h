#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantList>

#include "GPSObservation.h"
#include "GPSRelativePositionStore.h"

/// Fresh relative baseline and antenna heading, independently of position/course-over-ground.
class GPSRelativePositionModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sourceId READ sourceId NOTIFY stateChanged)
    Q_PROPERTY(qulonglong sessionId READ sessionId NOTIFY stateChanged)
    Q_PROPERTY(bool fresh READ fresh NOTIFY stateChanged)
    Q_PROPERTY(int referenceStationId READ referenceStationId NOTIFY stateChanged)
    Q_PROPERTY(double north READ north NOTIFY stateChanged)
    Q_PROPERTY(double east READ east NOTIFY stateChanged)
    Q_PROPERTY(double down READ down NOTIFY stateChanged)
    Q_PROPERTY(double northAccuracy READ northAccuracy NOTIFY stateChanged)
    Q_PROPERTY(double eastAccuracy READ eastAccuracy NOTIFY stateChanged)
    Q_PROPERTY(double downAccuracy READ downAccuracy NOTIFY stateChanged)
    Q_PROPERTY(double length READ length NOTIFY stateChanged)
    Q_PROPERTY(double lengthAccuracy READ lengthAccuracy NOTIFY stateChanged)
    Q_PROPERTY(double heading READ heading NOTIFY stateChanged)
    Q_PROPERTY(double headingAccuracy READ headingAccuracy NOTIFY stateChanged)
    Q_PROPERTY(bool fixValid READ fixValid NOTIFY stateChanged)
    Q_PROPERTY(bool differential READ differential NOTIFY stateChanged)
    Q_PROPERTY(bool positionValid READ positionValid NOTIFY stateChanged)
    Q_PROPERTY(bool carrierFloat READ carrierFloat NOTIFY stateChanged)
    Q_PROPERTY(bool carrierFixed READ carrierFixed NOTIFY stateChanged)
    Q_PROPERTY(QVariant movingBase READ movingBase NOTIFY stateChanged)
    Q_PROPERTY(QVariant referencePositionMissing READ referencePositionMissing NOTIFY stateChanged)
    Q_PROPERTY(QVariant referenceObservationsMissing READ referenceObservationsMissing NOTIFY stateChanged)
    Q_PROPERTY(QVariant normalized READ normalized NOTIFY stateChanged)

public:
    explicit GPSRelativePositionModel(QObject* parent = nullptr, int freshnessTimeoutMs = 5000,
                                      GPSRuntimeScheduler* scheduler = nullptr);
    ~GPSRelativePositionModel() override;

    QString sourceId() const { return _sourceId; }

    qulonglong sessionId() const { return _sessionId; }

    bool fresh() const { return _fresh; }

    int referenceStationId() const { return _fresh ? _observation.referenceStationId.value_or(-1) : -1; }

    double north() const;
    double east() const;
    double down() const;
    double northAccuracy() const;
    double eastAccuracy() const;
    double downAccuracy() const;
    double length() const;
    double lengthAccuracy() const;
    double heading() const;
    double headingAccuracy() const;

    bool fixValid() const { return _fresh && _observation.fixValid; }

    bool differential() const { return _fresh && _observation.differential; }

    bool positionValid() const { return _fresh && _observation.positionValid; }

    bool carrierFloat() const { return _fresh && _observation.carrierFloat; }

    bool carrierFixed() const { return _fresh && _observation.carrierFixed; }

    QVariant movingBase() const
    {
        return _fresh && _observation.movingBase.has_value() ? QVariant(*_observation.movingBase) : QVariant();
    }

    QVariant referencePositionMissing() const
    {
        return _fresh && _observation.referencePositionMissing.has_value()
                   ? QVariant(*_observation.referencePositionMissing)
                   : QVariant();
    }

    QVariant referenceObservationsMissing() const
    {
        return _fresh && _observation.referenceObservationsMissing.has_value()
                   ? QVariant(*_observation.referenceObservationsMissing)
                   : QVariant();
    }

    QVariant normalized() const
    {
        return _fresh && _observation.normalized.has_value() ? QVariant(*_observation.normalized) : QVariant();
    }

    void beginSession(const QString& sourceId, quint64 sessionId);
    void updateObservation(const GPSRelativeObservation& observation);
    void reset();

signals:
    void stateChanged();

private:
    double _positionValue(double value, bool accuracy = false) const;
    void _project();
    QVariantList _values() const;

    QString _sourceId;
    quint64 _sessionId = 0;
    GPSRelativeObservation _observation;
    GPSRelativePositionStore _store;
    bool _fresh = false;
};
