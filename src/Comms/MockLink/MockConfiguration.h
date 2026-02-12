#pragma once

#include "LinkConfiguration.h"

#include <QtCore/QLoggingCategory>
#include "MAVLinkLib.h"

Q_DECLARE_LOGGING_CATEGORY(MockConfigurationLog)

class MockConfiguration : public LinkConfiguration
{
    Q_OBJECT
    Q_PROPERTY(int  firmware                             READ firmware                            WRITE setFirmware                            NOTIFY firmwareChanged)
    Q_PROPERTY(int  vehicle                              READ vehicle                             WRITE setVehicle                             NOTIFY vehicleChanged)
    Q_PROPERTY(bool sendStatus                           READ sendStatusText                      WRITE setSendStatusText                      NOTIFY sendStatusChanged)
    Q_PROPERTY(bool enableCamera                         READ enableCamera                        WRITE setEnableCamera                        NOTIFY enableCameraChanged)
    Q_PROPERTY(bool incrementVehicleId                   READ incrementVehicleId                  WRITE setIncrementVehicleId                  NOTIFY incrementVehicleIdChanged)
    Q_PROPERTY(bool cameraCaptureVideo                   READ cameraCaptureVideo                  WRITE setCameraCaptureVideo                  NOTIFY cameraCaptureVideoChanged)
    Q_PROPERTY(bool cameraCaptureImage                   READ cameraCaptureImage                  WRITE setCameraCaptureImage                  NOTIFY cameraCaptureImageChanged)
    Q_PROPERTY(bool cameraHasModes                       READ cameraHasModes                      WRITE setCameraHasModes                      NOTIFY cameraHasModesChanged)
    Q_PROPERTY(bool cameraHasVideoStream                 READ cameraHasVideoStream                WRITE setCameraHasVideoStream                NOTIFY cameraHasVideoStreamChanged)
    Q_PROPERTY(bool cameraCanCaptureImageInVideoMode     READ cameraCanCaptureImageInVideoMode    WRITE setCameraCanCaptureImageInVideoMode    NOTIFY cameraCanCaptureImageInVideoModeChanged)
    Q_PROPERTY(bool cameraCanCaptureVideoInImageMode     READ cameraCanCaptureVideoInImageMode    WRITE setCameraCanCaptureVideoInImageMode    NOTIFY cameraCanCaptureVideoInImageModeChanged)
    Q_PROPERTY(bool cameraHasBasicZoom                   READ cameraHasBasicZoom                  WRITE setCameraHasBasicZoom                  NOTIFY cameraHasBasicZoomChanged)
    Q_PROPERTY(bool cameraHasTrackingPoint               READ cameraHasTrackingPoint              WRITE setCameraHasTrackingPoint              NOTIFY cameraHasTrackingPointChanged)
    Q_PROPERTY(bool cameraHasTrackingRectangle           READ cameraHasTrackingRectangle          WRITE setCameraHasTrackingRectangle          NOTIFY cameraHasTrackingRectangleChanged)

public:
    explicit MockConfiguration(const QString &name, QObject *parent = nullptr);
    explicit MockConfiguration(const MockConfiguration *copy, QObject *parent = nullptr);
    ~MockConfiguration();

    LinkType type() const final { return LinkConfiguration::TypeMock; }
    void copyFrom(const LinkConfiguration *source) final;
    void loadSettings(QSettings &settings, const QString &root) final;
    void saveSettings(QSettings &settings, const QString &root) const final;
    QString settingsURL() const final { return QStringLiteral("MockLinkSettings.qml"); }
    QString settingsTitle() const final { return tr("Mock Link Settings"); }

    int firmware() const { return static_cast<int>(_firmwareType); }
    void setFirmware(int type) { _firmwareType = static_cast<MAV_AUTOPILOT>(type); emit firmwareChanged(); }
    int vehicle() const { return static_cast<int>(_vehicleType); }
    void setVehicle(int type) { _vehicleType = static_cast<MAV_TYPE>(type); emit vehicleChanged(); }
    bool incrementVehicleId() const { return _incrementVehicleId; }
    void setIncrementVehicleId(bool incrementVehicleId) { _incrementVehicleId = incrementVehicleId; emit incrementVehicleIdChanged(); }
    MAV_AUTOPILOT firmwareType() const { return _firmwareType; }
    void setFirmwareType(MAV_AUTOPILOT firmwareType) { _firmwareType = firmwareType; emit firmwareChanged(); }
    uint16_t boardVendorId() const { return _boardVendorId; }
    uint16_t boardProductId() const { return _boardProductId; }
    void setBoardVendorProduct(uint16_t vendorId, uint16_t productId) { _boardVendorId = vendorId; _boardProductId = productId; }
    MAV_TYPE vehicleType() const { return _vehicleType; }
    void setVehicleType(MAV_TYPE vehicleType) { _vehicleType = vehicleType; emit vehicleChanged(); }
    bool sendStatusText() const { return _sendStatusText; }
    void setSendStatusText(bool sendStatusText) { _sendStatusText = sendStatusText; emit sendStatusChanged(); }
    bool enableCamera() const { return _enableCamera; }
    void setEnableCamera(bool enableCamera) { _enableCamera = enableCamera; emit enableCameraChanged(); }

    bool cameraCaptureVideo() const { return _cameraCaptureVideo; }
    void setCameraCaptureVideo(bool value) { _cameraCaptureVideo = value; emit cameraCaptureVideoChanged(); }
    bool cameraCaptureImage() const { return _cameraCaptureImage; }
    void setCameraCaptureImage(bool value) { _cameraCaptureImage = value; emit cameraCaptureImageChanged(); }
    bool cameraHasModes() const { return _cameraHasModes; }
    void setCameraHasModes(bool value) { _cameraHasModes = value; emit cameraHasModesChanged(); }
    bool cameraHasVideoStream() const { return _cameraHasVideoStream; }
    void setCameraHasVideoStream(bool value) { _cameraHasVideoStream = value; emit cameraHasVideoStreamChanged(); }
    bool cameraCanCaptureImageInVideoMode() const { return _cameraCanCaptureImageInVideoMode; }
    void setCameraCanCaptureImageInVideoMode(bool value) { _cameraCanCaptureImageInVideoMode = value; emit cameraCanCaptureImageInVideoModeChanged(); }
    bool cameraCanCaptureVideoInImageMode() const { return _cameraCanCaptureVideoInImageMode; }
    void setCameraCanCaptureVideoInImageMode(bool value) { _cameraCanCaptureVideoInImageMode = value; emit cameraCanCaptureVideoInImageModeChanged(); }
    bool cameraHasBasicZoom() const { return _cameraHasBasicZoom; }
    void setCameraHasBasicZoom(bool value) { _cameraHasBasicZoom = value; emit cameraHasBasicZoomChanged(); }
    bool cameraHasTrackingPoint() const { return _cameraHasTrackingPoint; }
    void setCameraHasTrackingPoint(bool value) { _cameraHasTrackingPoint = value; emit cameraHasTrackingPointChanged(); }
    bool cameraHasTrackingRectangle() const { return _cameraHasTrackingRectangle; }
    void setCameraHasTrackingRectangle(bool value) { _cameraHasTrackingRectangle = value; emit cameraHasTrackingRectangleChanged(); }

    enum FailureMode_t {
        FailNone,                                                   ///< No failures
        FailParamNoResponseToRequestList,                           ///< Do not respond to PARAM_REQUEST_LIST
        FailMissingParamOnInitialRequest,                           ///< Not all params are sent on initial request, should still succeed since QGC will re-query missing params
        FailMissingParamOnAllRequests,                              ///< Not all params are sent on initial request, QGC retries will fail as well
        FailInitialConnectRequestMessageAutopilotVersionFailure,    ///< REQUEST_MESSAGE:AUTOPILOT_VERSION returns failure
        FailInitialConnectRequestMessageAutopilotVersionLost,       ///< REQUEST_MESSAGE:AUTOPILOT_VERSION success, AUTOPILOT_VERSION never sent
    };
    FailureMode_t failureMode() const { return _failureMode; }
    void setFailureMode(FailureMode_t failureMode) { _failureMode = failureMode; }

signals:
    void firmwareChanged();
    void vehicleChanged();
    void sendStatusChanged();
    void enableCameraChanged();
    void incrementVehicleIdChanged();
    void cameraCaptureVideoChanged();
    void cameraCaptureImageChanged();
    void cameraHasModesChanged();
    void cameraHasVideoStreamChanged();
    void cameraCanCaptureImageInVideoModeChanged();
    void cameraCanCaptureVideoInImageModeChanged();
    void cameraHasBasicZoomChanged();
    void cameraHasTrackingPointChanged();
    void cameraHasTrackingRectangleChanged();

private:
    MAV_AUTOPILOT _firmwareType = MAV_AUTOPILOT_PX4;
    MAV_TYPE _vehicleType = MAV_TYPE_QUADROTOR;
    bool _sendStatusText = false;
    bool _enableCamera = false;
    FailureMode_t _failureMode = FailNone;
    bool _incrementVehicleId = true;
    uint16_t _boardVendorId = 0;
    uint16_t _boardProductId = 0;

    // Camera capability flags (defaults match current Camera 1 configuration)
    bool _cameraCaptureVideo = true;
    bool _cameraCaptureImage = true;
    bool _cameraHasModes = true;
    bool _cameraHasVideoStream = true;
    bool _cameraCanCaptureImageInVideoMode = true;
    bool _cameraCanCaptureVideoInImageMode = false;
    bool _cameraHasBasicZoom = true;
    bool _cameraHasTrackingPoint = false;
    bool _cameraHasTrackingRectangle = false;

    static constexpr const char *_firmwareTypeKey = "FirmwareType";
    static constexpr const char *_vehicleTypeKey = "VehicleType";
    static constexpr const char *_sendStatusTextKey = "SendStatusText";
    static constexpr const char *_enableCameraKey = "EnableCamera";
    static constexpr const char *_incrementVehicleIdKey = "IncrementVehicleId";
    static constexpr const char *_failureModeKey = "FailureMode";
    static constexpr const char *_cameraCaptureVideoKey = "CameraCaptureVideo";
    static constexpr const char *_cameraCaptureImageKey = "CameraCaptureImage";
    static constexpr const char *_cameraHasModesKey = "CameraHasModes";
    static constexpr const char *_cameraHasVideoStreamKey = "CameraHasVideoStream";
    static constexpr const char *_cameraCanCaptureImageInVideoModeKey = "CameraCanCaptureImageInVideoMode";
    static constexpr const char *_cameraCanCaptureVideoInImageModeKey = "CameraCanCaptureVideoInImageMode";
    static constexpr const char *_cameraHasBasicZoomKey = "CameraHasBasicZoom";
    static constexpr const char *_cameraHasTrackingPointKey = "CameraHasTrackingPoint";
    static constexpr const char *_cameraHasTrackingRectangleKey = "CameraHasTrackingRectangle";
};
