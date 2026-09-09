#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "GPSObservation.h"

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
    Q_PROPERTY(bool movingBase READ movingBase NOTIFY stateChanged)
    Q_PROPERTY(bool referencePositionMissing READ referencePositionMissing NOTIFY stateChanged)
    Q_PROPERTY(bool referenceObservationsMissing READ referenceObservationsMissing NOTIFY stateChanged)
    Q_PROPERTY(bool normalized READ normalized NOTIFY stateChanged)

public:
    explicit GPSRelativePositionModel(QObject* parent = nullptr, int freshnessTimeoutMs = 5000);
    ~GPSRelativePositionModel() override;

    QString sourceId() const { return _sourceId; }

    qulonglong sessionId() const { return _sessionId; }

    bool fresh() const { return _fresh; }

    int referenceStationId() const { return _fresh ? _observation.referenceStationId : -1; }

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

    bool movingBase() const { return _fresh && _observation.movingBase; }

    bool referencePositionMissing() const { return _fresh && _observation.referencePositionMissing; }

    bool referenceObservationsMissing() const { return _fresh && _observation.referenceObservationsMissing; }

    bool normalized() const { return _fresh && _observation.normalized; }

    void beginSession(const QString& sourceId, quint64 sessionId);
    void updateObservation(const GPSRelativeObservation& observation);
    void reset();

signals:
    void stateChanged();

private:
    double _positionValue(double value, bool accuracy = false) const;
    void _expire();
    void _armTimer();

    QString _sourceId;
    quint64 _sessionId = 0;
    GPSRelativeObservation _observation;
    QTimer _expiryTimer;
    int _freshnessTimeoutMs;
    bool _fresh = false;
};
