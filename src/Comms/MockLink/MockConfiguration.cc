#include "MockConfiguration.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MockConfigurationLog, "Comms.MockLink.MockConfiguration")

MockConfiguration::MockConfiguration(const QString &name, QObject *parent)
    : LinkConfiguration(name, parent)
{
    qCDebug(MockConfigurationLog) << this;
}

MockConfiguration::MockConfiguration(const MockConfiguration *copy, QObject *parent)
    : LinkConfiguration(copy, parent)
    , _firmwareType(copy->firmwareType())
    , _vehicleType(copy->vehicleType())
    , _sendStatusText(copy->sendStatusText())
    , _enableCamera(copy->enableCamera())
    , _failureMode(copy->failureMode())
    , _incrementVehicleId(copy->incrementVehicleId())
    , _cameraCaptureVideo(copy->cameraCaptureVideo())
    , _cameraCaptureImage(copy->cameraCaptureImage())
    , _cameraHasModes(copy->cameraHasModes())
    , _cameraHasVideoStream(copy->cameraHasVideoStream())
    , _cameraCanCaptureImageInVideoMode(copy->cameraCanCaptureImageInVideoMode())
    , _cameraCanCaptureVideoInImageMode(copy->cameraCanCaptureVideoInImageMode())
    , _cameraHasBasicZoom(copy->cameraHasBasicZoom())
    , _cameraHasTrackingPoint(copy->cameraHasTrackingPoint())
    , _cameraHasTrackingRectangle(copy->cameraHasTrackingRectangle())
{
    qCDebug(MockConfigurationLog) << this;
}

MockConfiguration::~MockConfiguration()
{
    qCDebug(MockConfigurationLog) << this;
}

void MockConfiguration::copyFrom(const LinkConfiguration *source)
{
    LinkConfiguration::copyFrom(source);

    const MockConfiguration *mockLinkSource = qobject_cast<const MockConfiguration*>(source);

    setFirmwareType(mockLinkSource->firmwareType());
    setVehicleType(mockLinkSource->vehicleType());
    setSendStatusText(mockLinkSource->sendStatusText());
    setEnableCamera(mockLinkSource->enableCamera());
    setIncrementVehicleId(mockLinkSource->incrementVehicleId());
    setFailureMode(mockLinkSource->failureMode());
    setCameraCaptureVideo(mockLinkSource->cameraCaptureVideo());
    setCameraCaptureImage(mockLinkSource->cameraCaptureImage());
    setCameraHasModes(mockLinkSource->cameraHasModes());
    setCameraHasVideoStream(mockLinkSource->cameraHasVideoStream());
    setCameraCanCaptureImageInVideoMode(mockLinkSource->cameraCanCaptureImageInVideoMode());
    setCameraCanCaptureVideoInImageMode(mockLinkSource->cameraCanCaptureVideoInImageMode());
    setCameraHasBasicZoom(mockLinkSource->cameraHasBasicZoom());
    setCameraHasTrackingPoint(mockLinkSource->cameraHasTrackingPoint());
    setCameraHasTrackingRectangle(mockLinkSource->cameraHasTrackingRectangle());
}

void MockConfiguration::loadSettings(QSettings &settings, const QString &root)
{
    settings.beginGroup(root);

    setFirmwareType(static_cast<MAV_AUTOPILOT>(settings.value(_firmwareTypeKey, static_cast<int>(MAV_AUTOPILOT_PX4)).toInt()));
    setVehicleType(static_cast<MAV_TYPE>(settings.value(_vehicleTypeKey, static_cast<int>(MAV_TYPE_QUADROTOR)).toInt()));
    setSendStatusText(settings.value(_sendStatusTextKey, false).toBool());
    setEnableCamera(settings.value(_enableCameraKey, false).toBool());
    setIncrementVehicleId(settings.value(_incrementVehicleIdKey, true).toBool());
    setFailureMode(static_cast<FailureMode_t>(settings.value(_failureModeKey, static_cast<int>(FailNone)).toInt()));
    setCameraCaptureVideo(settings.value(_cameraCaptureVideoKey, true).toBool());
    setCameraCaptureImage(settings.value(_cameraCaptureImageKey, true).toBool());
    setCameraHasModes(settings.value(_cameraHasModesKey, true).toBool());
    setCameraHasVideoStream(settings.value(_cameraHasVideoStreamKey, true).toBool());
    setCameraCanCaptureImageInVideoMode(settings.value(_cameraCanCaptureImageInVideoModeKey, true).toBool());
    setCameraCanCaptureVideoInImageMode(settings.value(_cameraCanCaptureVideoInImageModeKey, false).toBool());
    setCameraHasBasicZoom(settings.value(_cameraHasBasicZoomKey, true).toBool());
    setCameraHasTrackingPoint(settings.value(_cameraHasTrackingPointKey, false).toBool());
    setCameraHasTrackingRectangle(settings.value(_cameraHasTrackingRectangleKey, false).toBool());

    settings.endGroup();
}

void MockConfiguration::saveSettings(QSettings &settings, const QString &root) const
{
    settings.beginGroup(root);

    settings.setValue(_firmwareTypeKey, firmwareType());
    settings.setValue(_vehicleTypeKey, vehicleType());
    settings.setValue(_sendStatusTextKey, sendStatusText());
    settings.setValue(_enableCameraKey, enableCamera());
    settings.setValue(_incrementVehicleIdKey, incrementVehicleId());
    settings.setValue(_failureModeKey, failureMode());
    settings.setValue(_cameraCaptureVideoKey, cameraCaptureVideo());
    settings.setValue(_cameraCaptureImageKey, cameraCaptureImage());
    settings.setValue(_cameraHasModesKey, cameraHasModes());
    settings.setValue(_cameraHasVideoStreamKey, cameraHasVideoStream());
    settings.setValue(_cameraCanCaptureImageInVideoModeKey, cameraCanCaptureImageInVideoMode());
    settings.setValue(_cameraCanCaptureVideoInImageModeKey, cameraCanCaptureVideoInImageMode());
    settings.setValue(_cameraHasBasicZoomKey, cameraHasBasicZoom());
    settings.setValue(_cameraHasTrackingPointKey, cameraHasTrackingPoint());
    settings.setValue(_cameraHasTrackingRectangleKey, cameraHasTrackingRectangle());

    settings.endGroup();
}
