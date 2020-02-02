/*!
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

/// @file
/// @brief  MAVLink Camera API. Camera parameter handler.
/// @author Gus Grubba <gus@auterion.com>

#pragma once

#include "QGCApplication.h"
#include <QLoggingCategory>

class QGCCameraControl;

Q_DECLARE_LOGGING_CATEGORY(CameraIOLog)
Q_DECLARE_LOGGING_CATEGORY(CameraIOLogVerbose)

MAVPACKED(
typedef struct {
    union {
        float       param_float;
        double      param_double;
        int64_t     param_int64;
        uint64_t    param_uint64;
        int32_t     param_int32;
        uint32_t    param_uint32;
        int16_t     param_int16;
        uint16_t    param_uint16;
        int8_t      param_int8;
        uint8_t     param_uint8;
        uint8_t     bytes[MAVLINK_MSG_PARAM_EXT_SET_FIELD_PARAM_VALUE_LEN];
    };
    uint8_t type;
}) param_ext_union_t;

//-----------------------------------------------------------------------------
/// Camera parameter handler.
class QGCCameraParamIO : public QObject
{
public:
    QGCCameraParamIO(QGCCameraControl* control, Fact* fact, Vehicle* vehicle);

    void        handleParamAck              (const mavlink_param_ext_ack_t& ack);
    void        handleParamValue            (const mavlink_param_ext_value_t& value);
    void        setParamRequest             ();
    bool        paramDone                   () { return _done; }
    void        paramRequest                (bool reset = true);
    void        sendParameter               (bool updateUI = false);

    QStringList  optNames;
    QVariantList optVariants;

private slots:
    void        _paramWriteTimeout          ();
    void        _paramRequestTimeout        ();
    void        _factChanged                (QVariant value);
    void        _containerRawValueChanged   (const QVariant value);

private:
    void        _sendParameter              ();
    QVariant    _valueFromMessage           (const char* value, uint8_t param_type);

private:
    QGCCameraControl*   _control;
    Fact*               _fact;
    Vehicle*            _vehicle;
    int                 _sentRetries;
    int                 _requestRetries;
    bool                _paramRequestReceived;
    QTimer              _paramWriteTimer;
    QTimer              _paramRequestTimer;
    bool                _done;
    bool                _updateOnSet;
    MAV_PARAM_EXT_TYPE  _mavParamType;
    MAVLinkProtocol*    _pMavlink;
    bool                _forceUIUpdate;
};

