#pragma once

#include <algorithm>
#include <memory>
#include <optional>

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

#include "Fact.h"
#include "QGCVideoStreamInfo.h"
#include "VideoReceiver.h"

/// Records what VideoReceiver's callers ask of it. Tests publish its state through the setters made public here.
class FakeVideoReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    using VideoReceiver::VideoReceiver;

    using VideoReceiver::setDecoding;
    using VideoReceiver::setRecording;
    using VideoReceiver::setState;
    using VideoReceiver::setStats;
    using VideoReceiver::setVideoSize;

    struct Recording
    {
        QString file;
        FileFormat format;
    };

    int configChanges = 0;
    int activeChanges = 0;
    int restarts = 0;
    int reconnects = 0;
    int videoOutputChanges = 0;
    QList<Recording> recordings;
    int recordingStops = 0;
    int screenshots = 0;

protected:
    void onConfigChanged() override { ++configChanges; }

    void onActiveChanged() override { ++activeChanges; }

    void onRestartRequested() override { ++restarts; }

    void onReconnectRequested() override { ++reconnects; }

    void onVideoOutputChanged() override { ++videoOutputChanges; }

    void onStartRecording(const QString& file, FileFormat format) override { recordings.append({file, format}); }

    void onStopRecording() override { ++recordingStops; }

    bool onTakeScreenshot(const QString& file, const VideoScreenshot::Exif& exif) override
    {
        ++screenshots;
        return VideoReceiver::onTakeScreenshot(file, exif);
    }
};

namespace VideoTestHelpers {

/// Sets settings Facts for a test and restores their original values when destroyed.
class FactRestorer
{
public:
    FactRestorer() = default;
    FactRestorer(const FactRestorer&) = delete;
    FactRestorer& operator=(const FactRestorer&) = delete;

    ~FactRestorer()
    {
        for (auto it = _originalValues.cbegin(); it != _originalValues.cend(); ++it) {
            it.key()->setRawValue(it.value());
        }
    }

    void set(Fact* fact, const QVariant& value)
    {
        if (!_originalValues.contains(fact)) {
            _originalValues.insert(fact, fact->rawValue());
        }
        fact->setRawValue(value);
    }

private:
    QHash<Fact*, QVariant> _originalValues;
};

/// A camera's VIDEO_STREAM_INFORMATION.
inline std::unique_ptr<QGCVideoStreamInfo> makeStreamInfo(VIDEO_STREAM_TYPE type, const char* uri,
                                                          VIDEO_STREAM_ENCODING encoding = VIDEO_STREAM_ENCODING_H264,
                                                          bool thermal = false, quint16 rotation = 0)
{
    mavlink_video_stream_information_t info{};
    info.type = type;
    info.encoding = encoding;
    info.rotation = rotation;
    info.flags = thermal ? VIDEO_STREAM_STATUS_FLAGS_THERMAL : 0;
    info.resolution_h = 1280;
    info.resolution_v = 720;
    info.hfov = 90;
    (void) qstrncpy(info.uri, uri, sizeof(info.uri));
    return std::make_unique<QGCVideoStreamInfo>(info);
}

/// A QML VideoOutput of 64 × 48, as created by @p engine.
inline std::unique_ptr<QQuickItem> createVideoOutput(QQmlEngine& engine)
{
    QQmlComponent component(&engine);
    component.setData(R"QML(
import QtQuick
import QtMultimedia

VideoOutput {
    width: 64
    height: 48
}
)QML",
                      QUrl(QStringLiteral("qrc:/VideoTestHelpers/VideoOutput.qml")));
    std::unique_ptr<QObject> object(component.create());
    return std::unique_ptr<QQuickItem>(qobject_cast<QQuickItem*>(object.release()));
}

/// 64 × 48, red in the top-left quadrant only.
inline QImage redTopLeftQuadrant()
{
    QImage image(QSize(64, 48), QImage::Format_RGBA8888);
    image.fill(Qt::blue);
    for (int y = 0; y < 24; ++y) {
        for (int x = 0; x < 32; ++x) {
            image.setPixelColor(x, y, Qt::red);
        }
    }
    return image;
}

/// The corner of @p image whose quadrant is the red one; nullopt unless exactly one is.
inline std::optional<Qt::Corner> redCorner(const QImage& image)
{
    std::optional<Qt::Corner> corner;
    for (const Qt::Corner candidate :
         {Qt::TopLeftCorner, Qt::TopRightCorner, Qt::BottomLeftCorner, Qt::BottomRightCorner}) {
        const bool right = (candidate == Qt::TopRightCorner) || (candidate == Qt::BottomRightCorner);
        const bool bottom = (candidate == Qt::BottomLeftCorner) || (candidate == Qt::BottomRightCorner);
        const QColor color =
            image.pixelColor(image.width() * (right ? 3 : 1) / 4, image.height() * (bottom ? 3 : 1) / 4);
        if ((color.red() > 200) && (color.blue() < 50)) {
            if (corner) {
                return std::nullopt;
            }
            corner = candidate;
        }
    }
    return corner;
}

/// Hands @p videoOutput's sink @p image as its next frame.
inline void showFrame(QQuickItem* videoOutput, const QImage& image)
{
    videoOutput->property("videoSink").value<QVideoSink*>()->setVideoFrame(QVideoFrame(image));
}

/// Whether @p root shows a label reading @p text.
inline bool showsText(const QQuickItem* root, const QString& text)
{
    const QList<QQuickItem*> items = root->findChildren<QQuickItem*>();
    return std::ranges::any_of(
        items, [&text](const QQuickItem* item) { return item->isVisible() && (item->property("text") == text); });
}

}  // namespace VideoTestHelpers
