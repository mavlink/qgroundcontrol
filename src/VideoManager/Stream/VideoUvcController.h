#pragma once

#include <QtCore/QObject>

#include <functional>

class VideoStream;
class VideoStreamModel;

/// Owns the local-camera handoff between the network primary stream and UVC.
class VideoUvcController : public QObject
{
    Q_OBJECT

public:
    using EnsureStreamFn = std::function<VideoStream*()>;
    using LocalCameraAvailableFn = std::function<bool()>;
    using RemoveStreamFn = std::function<void()>;

    explicit VideoUvcController(VideoStreamModel* streamModel, QObject* parent = nullptr);

    void setLocalCameraAvailable(LocalCameraAvailableFn fn) { _localCameraAvailable = std::move(fn); }
    [[nodiscard]] bool localCameraAvailable() const;

    bool activate(VideoStream* primaryStream, EnsureStreamFn ensureUvcStream);
    bool deactivate(VideoStream* uvcStream, RemoveStreamFn removeUvcStream);

private:
    VideoStreamModel* _streamModel = nullptr;
    LocalCameraAvailableFn _localCameraAvailable;
};
