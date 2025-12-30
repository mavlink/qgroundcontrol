/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QTimer>

#include "MAVLinkLib.h"

class MavlinkCameraControl;
class Fact;
class Vehicle;

Q_DECLARE_LOGGING_CATEGORY(CameraIOLog)
Q_DECLARE_LOGGING_CATEGORY(CameraIOLogVerbose)

/// Camera parameter handler.
class QGCCameraParamIO : public QObject
{
public:
    QGCCameraParamIO(MavlinkCameraControl *control, Fact *fact, Vehicle *vehicle);
    ~QGCCameraParamIO();

    void handleParamAck(const mavlink_param_ext_ack_t &ack);
    void handleParamValue(const mavlink_param_ext_value_t &value);
    void setParamRequest();
    bool paramDone() const { return _done; }
    void paramRequest(bool reset = true);
    void sendParameter(bool updateUI = false);

private slots:
    void _paramWriteTimeout();
    void _paramRequestTimeout();
    void _factChanged(const QVariant &value);
    void _containerRawValueChanged(const QVariant &value);

private:
    void _sendParameter();
    QVariant _valueFromMessage(const char *value, uint8_t param_type);

    MavlinkCameraControl *_control = nullptr;
    Fact *_fact = nullptr;
    Vehicle *_vehicle = nullptr;

    bool _done = false;
    bool _forceUIUpdate = false;
    bool _paramRequestReceived = false;
    bool _updateOnSet = false;
    int _requestRetries = 0;
    int _sentRetries = 0;
    MAV_PARAM_EXT_TYPE _mavParamType = MAV_PARAM_EXT_TYPE_UINT8;
    QTimer _paramRequestTimer;
    QTimer _paramWriteTimer;
};
