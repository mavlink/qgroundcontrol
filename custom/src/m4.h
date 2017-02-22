/*!
 * @file
 *   @brief ST16 Controller
 *   @author Gus Grubba <mavlink@grubba.com>
 *
 */

#pragma once

#include <QObject>
#include <QTimer>
#include "m4def.h"
#include "m4util.h"

#include "Vehicle.h"

class QGCToolbox;
class TyphoonM4Handler;

//-----------------------------------------------------------------------------
// QtQuick Interface (UI)
class TyphoonHQuickInterface : public QObject
{
    Q_OBJECT
public:
    TyphoonHQuickInterface(QObject* parent = NULL);
    ~TyphoonHQuickInterface() {}

    //-- QtQuick Interface
    enum M4State {
        M4_STATE_NONE           = 0,
        M4_STATE_AWAIT          = 1,
        M4_STATE_BIND           = 2,
        M4_STATE_CALIBRATION    = 3,
        M4_STATE_SETUP          = 4,
        M4_STATE_RUN            = 5,
        M4_STATE_SIM            = 6,
        M4_STATE_FACTORY_CALI   = 7
    };

    //-- Camera Control (to be moved to Vehicle)
    enum VideoStatus {
        VIDEO_CAPTURE_STATUS_UNDEFINED,
        VIDEO_CAPTURE_STATUS_STOPPED,
        VIDEO_CAPTURE_STATUS_RUNNING,
    };

    enum CameraMode {
        CAMERA_MODE_UNDEFINED,
        CAMERA_MODE_PHOTO,
        CAMERA_MODE_VIDEO,
    };

    Q_ENUMS(M4State)
    Q_ENUMS(VideoStatus)
    Q_ENUMS(CameraMode)

    Q_PROPERTY(M4State      m4State     READ    m4State                             NOTIFY m4StateChanged)
    Q_PROPERTY(QString      m4StateStr  READ    m4StateStr                          NOTIFY m4StateChanged)
    Q_PROPERTY(VideoStatus  videoStatus READ    videoStatus                         NOTIFY videoStatusChanged)
    Q_PROPERTY(CameraMode   cameraMode  READ    cameraMode  WRITE   setCameraMode   NOTIFY cameraModeChanged)
    Q_PROPERTY(QString      recordTime  READ    recordTime                          NOTIFY recordTimeChanged)

    Q_INVOKABLE void enterBindMode  ();
    Q_INVOKABLE void initM4         ();
    Q_INVOKABLE void startVideo     ();
    Q_INVOKABLE void stopVideo      ();
    Q_INVOKABLE void takePhoto      ();
    Q_INVOKABLE void toggleMode     ();

    M4State     m4State             ();
    QString     m4StateStr          ();
    VideoStatus videoStatus         ();
    CameraMode  cameraMode          ();
    QString     recordTime          ();

    void        setCameraMode       (CameraMode mode);

    void    init                    (TyphoonM4Handler* pHandler);

signals:
    void    m4StateChanged          ();
    void    videoStatusChanged      ();
    void    cameraModeChanged       ();
    void    recordTimeChanged       ();

private slots:
    void    _m4StateChanged         ();
    void    _destroyed              ();
    void    _cameraModeChanged      ();
    void    _videoStatusChanged     ();
    void    _videoRecordingUpdate   ();

private:
    TyphoonM4Handler* _pHandler;
    QTimer            _videoRecordingTimer;

};

//-----------------------------------------------------------------------------
// M4 Handler
class TyphoonM4Handler : public QObject
{
    Q_OBJECT
public:
    TyphoonM4Handler(QObject* parent = NULL);
    ~TyphoonM4Handler();

    void    init                    ();
    bool    vehicleReady            ();
    void    getControllerLocation   (ControllerLocation& location);
    void    enterBindMode           ();
    void    initM4                  ();
    void    toggleMode              ();
    void    takePhoto               ();
    void    toggleVideo             ();
    void    startVideo              ();
    void    stopVideo               ();
    void    setVideoMode            ();
    void    setPhotoMode            ();
    QTime   recordTime              () { return _recordTime; }

    TyphoonHQuickInterface::M4State     m4State     () { return _m4State; }
    TyphoonHQuickInterface::VideoStatus videoStatus () { return _video_status; }
    TyphoonHQuickInterface::CameraMode  cameraMode  () { return _camera_mode; }

    static  int     byteArrayToInt  (QByteArray data, int offset, bool isBigEndian = false);
    static  float   byteArrayToFloat(QByteArray data, int offset);
    static  short   byteArrayToShort(QByteArray data, int offset, bool isBigEndian = false);

public slots:
    void    softReboot                          ();

private slots:
    void    _bytesReady                         (QByteArray data);
    void    _stateManager                       ();
    void    _initSequence                       ();
    void    _vehicleAdded                       (Vehicle* vehicle);
    void    _vehicleRemoved                     (Vehicle* vehicle);
    void    _vehicleReady                       (bool ready);
    void    _mavlinkMessageReceived             (const mavlink_message_t& message);
    void    _videoCaptureUpdate                 ();
    void    _requestCameraSettings              ();

private:
    bool    _enterRun                           ();
    bool    _exitRun                            ();
    bool    _startBind                          ();
    bool    _enterBind                          ();
    bool    _exitBind                           ();
    bool    _bind                               (int rxAddr);
    bool    _unbind                             ();
    void    _checkExitRun                       ();
    bool    _queryBindState                     ();
    bool    _sendRecvBothCh                     ();
    bool    _setChannelSetting                  ();
    bool    _syncMixingDataDeleteAll            ();
    bool    _syncMixingDataAdd                  ();
    bool    _sendRxResInfo                      ();
    bool    _sendTableDeviceLocalInfo           (TableDeviceLocalInfo_t localInfo);
    bool    _sendTableDeviceChannelInfo         (TableDeviceChannelInfo_t channelInfo);
    void    _generateTableDeviceLocalInfo       (TableDeviceLocalInfo_t *localInfo);
    bool    _generateTableDeviceChannelInfo     (TableDeviceChannelInfo_t *channelInfo);
    bool    _sendTableDeviceChannelNumInfo      (ChannelNumType_t channelNumTpye);
    bool    _generateTableDeviceChannelNumInfo  (TableDeviceChannelNumInfo_t *channelNumInfo, ChannelNumType_t channelNumTpye, int& num);
    bool    _fillTableDeviceChannelNumMap       (TableDeviceChannelNumInfo_t *channelNumInfo, int num, QByteArray list);
    bool    _setPowerKey                        (int function);
    void    _handleBindResponse                 ();
    void    _handleQueryBindResponse            (QByteArray data);
    bool    _handleNonTypePacket                (m4Packet& packet);
    void    _handleRxBindInfo                   (m4Packet& packet);
    void    _handleChannel                      (m4Packet& packet);
    bool    _handleCommand                      (m4Packet& packet);
    void    _switchChanged                      (m4Packet& packet);
    void    _handleMixedChannelData             (m4Packet& packet);
    void    _handControllerFeedback             (m4Packet& packet);
    void    _handleInitialState                 ();

    //-- Camera Control (to be moved to Vehicle)
    void    _requestCaptureStatus               ();
    void    _handleCaptureStatus                (const mavlink_message_t& message);
    void    _handleCameraSettings               (const mavlink_message_t& message);

signals:
    void    m4StateChanged                      ();
    void    switchStateChanged                  (int swId, int oldState, int newState);
    void    channelDataStatus                   (QByteArray channelData);
    void    controllerLocationChanged           ();
    void    destroyed                           ();

    //-- Camera Control (to be moved to Vehicle)
    void    cameraModeChanged                   ();
    void    videoStatusChanged                  ();

private:
    M4SerialComm* _commPort;
    enum {
        STATE_NONE,
        STATE_ENTER_BIND_ERROR,
        STATE_EXIT_RUN,
        STATE_ENTER_BIND,
        STATE_START_BIND,
        STATE_UNBIND,
        STATE_BIND,
        STATE_QUERY_BIND,
        STATE_EXIT_BIND,
        STATE_RECV_BOTH_CH,
        STATE_SET_CHANNEL_SETTINGS,
        STATE_MIX_CHANNEL_DELETE,
        STATE_MIX_CHANNEL_ADD,
        STATE_SEND_RX_INFO,
        STATE_ENTER_RUN,
        STATE_RUNNING
    };
    int                     _state;
    int                     _responseTryCount;
    int                     _currentChannelAdd;
    uint8_t                 _rxLocalIndex;
    uint8_t                 _rxchannelInfoIndex;
    uint8_t                 _channelNumIndex;
    bool                    _sendRxInfoEnd;
    RxBindInfo              _rxBindInfoFeedback;
    QTimer                  _timer;
    ControllerLocation      _controllerLocation;
    bool                    _binding;
    Vehicle*                _vehicle;

    TyphoonHQuickInterface::M4State     _m4State;
    TyphoonHQuickInterface::VideoStatus _video_status;
    int                                 _video_resolution_h;
    int                                 _video_resolution_v;
    float                               _video_framerate;
    TyphoonHQuickInterface::CameraMode  _camera_mode;
    QTimer                              _videoTimer;

    //-- This should come from the camera. In the mean time, we keep track of it here.
    QTime                               _recordTime;

};
