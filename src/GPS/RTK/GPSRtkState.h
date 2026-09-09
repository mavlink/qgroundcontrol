#pragma once

#include <QtCore/QObject>

#include "GPSRTKFactGroup.h"
#include "GPSReceiverSession.h"

/// Survey and fixed-base presentation; the independently owned session must outlive this object.
class GPSRtkState : public QObject
{
    Q_OBJECT

public:
    explicit GPSRtkState(GPSReceiverSession& session, QObject* parent = nullptr);
    ~GPSRtkState() override;

    GPSRTKFactGroup* facts() { return &_facts; }

private:
    bool _acceptsSurvey() const;
    void _updateSurvey(const GPSSurveyInStatus& status);
    void _reset();

    GPSReceiverSession& _session;
    GPSRTKFactGroup _facts;
    quint64 _revision = 0;
};
