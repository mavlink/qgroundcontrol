/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include "QGCApplication.h"
#include <QLoggingCategory>

class QGCCameraControl;

Q_DECLARE_LOGGING_CATEGORY(CameraIOLog)
Q_DECLARE_LOGGING_CATEGORY(CameraIOLogVerbose)

//-----------------------------------------------------------------------------
class QGCCameraParamIO : public QObject
{
public:
    QGCCameraParamIO(QGCCameraControl* control, Fact* fact, Vehicle* vehicle);

    void        handleParamAck          (const mavlink_param_ext_ack_t& ack);
    void        handleParamValue        (const mavlink_param_ext_value_t& value);
    void        setParamRequest         ();

private slots:
    void                _factChanged        (QVariant value);
    void                _containerRawValueChanged(const QVariant value);
    void                _paramWriteTimeout  ();
    void                _paramRequestTimeout();

private:
    void                _sendParameter      ();
    QVariant            _valueFromMessage   (const char* value, uint8_t param_type);
    MAV_PARAM_TYPE      _factTypeToMavType  (FactMetaData::ValueType_t factType);

private:
    QGCCameraControl*   _control;
    Fact*               _fact;
    Vehicle*            _vehicle;
    int                 _sentRetries;
    int                 _requestRetries;
    bool                _paramRequestReceived;
    mavlink_message_t   _msg;
    QTimer              _paramWriteTimer;
    QTimer              _paramRequestTimer;
};

