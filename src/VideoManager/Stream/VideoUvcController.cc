#include "VideoUvcController.h"

#include "VideoStream.h"
#include "VideoStreamModel.h"

VideoUvcController::VideoUvcController(VideoStreamModel* streamModel, QObject* parent)
    : QObject(parent),
      _streamModel(streamModel)
{
}

bool VideoUvcController::localCameraAvailable() const
{
    return !_localCameraAvailable || _localCameraAvailable();
}

bool VideoUvcController::activate(VideoStream* primaryStream, EnsureStreamFn ensureUvcStream)
{
    if (!localCameraAvailable() || !ensureUvcStream)
        return false;

    auto* uvcStream = ensureUvcStream();
    if (!uvcStream)
        return false;

    if (primaryStream)
        primaryStream->stop();

    uvcStream->setUri(QStringLiteral("uvc://local"));
    uvcStream->restart();

    if (_streamModel)
        _streamModel->setUvcActive(true);

    return true;
}

bool VideoUvcController::deactivate(VideoStream* uvcStream, RemoveStreamFn removeUvcStream)
{
    if (!uvcStream)
        return false;

    uvcStream->stop();
    uvcStream->setUri(QString());

    if (_streamModel)
        _streamModel->setUvcActive(false);
    if (removeUvcStream)
        removeUvcStream();

    return true;
}
