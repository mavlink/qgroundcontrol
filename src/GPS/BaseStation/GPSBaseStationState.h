#pragma once

#include <QtCore/QObject>

#include "GPSBaseStationFactGroup.h"
#include "GPSReceiverSession.h"

/// Survey and fixed-base presentation; the session and Facts must outlive this object.
class GPSBaseStationState : public QObject
{
    Q_OBJECT

public:
    explicit GPSBaseStationState(GPSReceiverSession& session, GPSBaseStationFactGroup& facts,
                                 QObject* parent = nullptr);
    ~GPSBaseStationState() override;

private:
    bool _acceptsSurvey() const;
    void _updateSurvey(const GPSSurveyInStatus& status);
    void _reset();

    GPSReceiverSession& _session;
    GPSBaseStationFactGroup& _facts;
    quint64 _revision = 0;
};
