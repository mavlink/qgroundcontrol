#include "VideoStreamModel.h"

#include "VideoFrameDelivery.h"
#include "VideoStream.h"

VideoStreamModel::VideoStreamModel(QObject* parent) : QAbstractListModel(parent) {}

void VideoStreamModel::setUvcActive(bool active)
{
    if (_uvcActive == active)
        return;
    _uvcActive = active;
    emit activeStreamChanged(static_cast<int>(VideoStream::Role::Primary));
}

VideoStream* VideoStreamModel::activeStreamForRole(int role) const
{
    if (role == static_cast<int>(VideoStream::Role::Primary) && _uvcActive)
        return streamForRole(static_cast<int>(VideoStream::Role::UVC));
    return streamForRole(role);
}

int VideoStreamModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : _streams.size();
}

QVariant VideoStreamModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= _streams.size())
        return {};

    const VideoStream* s = _streams.at(index.row());

    switch (role) {
        case NameRole:
            return s->name();
        case UriRole:
            return s->uri();
        case IsThermalRole:
            return s->isThermal();
        case IsActiveRole:
            return s->started();
        case FrameDeliveryRole:
            return QVariant::fromValue(s->frameDelivery());
        case LastErrorRole:
            return s->lastError();
        case StreamRole:
            return QVariant::fromValue(const_cast<VideoStream*>(s));
        default:
            return {};
    }
}

QHash<int, QByteArray> VideoStreamModel::roleNames() const
{
    return {
        {NameRole, "streamName"},   {UriRole, "streamUri"}, {IsThermalRole, "isThermal"},
        {IsActiveRole, "isActive"}, {FrameDeliveryRole, "frameDelivery"}, {LastErrorRole, "streamLastError"},
        {StreamRole, "stream"},
    };
}

void VideoStreamModel::addStream(VideoStream* stream)
{
    if (!stream || _streams.contains(stream))
        return;

    const int row = _streams.size();
    beginInsertRows({}, row, row);
    _streams.append(stream);
    endInsertRows();

    // Re-emit dataChanged with minimal role lists so delegate bindings only
    // re-evaluate what actually changed. Passing an empty/broad role vector
    // forces every binding on the row to re-eval each signal.
    (void)connect(stream, &VideoStream::uriChanged, this, [this, stream]() { _onStreamChanged(stream, {UriRole}); });
    (void)connect(stream, &VideoStream::decodingChanged, this,
                  [this, stream]() { _onStreamChanged(stream, {IsActiveRole}); });
    (void)connect(stream, &VideoStream::streamingChanged, this,
                  [this, stream]() { _onStreamChanged(stream, {IsActiveRole}); });
    (void)connect(stream, &VideoStream::frameDeliveryChanged, this,
                  [this, stream]() { _onStreamChanged(stream, {FrameDeliveryRole}); });
    (void)connect(stream, &VideoStream::lastErrorChanged, this,
                  [this, stream](const QString&) { _onStreamChanged(stream, {LastErrorRole}); });

    // Notify QML consumers that the active stream for this role is now
    // available. QGCVideoOutput's Component.onCompleted may have fired before
    // streams were created (streamForRole returned null then); it relies on
    // activeStreamChanged to retry sink registration. Without this emission,
    // boot-at-RTSP leaves the sink unregistered on the videoContent stream
    // and frames flow into a detached frame delivery.
    emit activeStreamChanged(static_cast<int>(stream->role()));
}

void VideoStreamModel::removeStream(VideoStream* stream)
{
    const int row = _indexOf(stream);
    if (row < 0)
        return;

    beginRemoveRows({}, row, row);
    _streams.removeAt(row);
    endRemoveRows();

    disconnect(stream, nullptr, this, nullptr);
}

VideoStream* VideoStreamModel::stream(const QString& name) const
{
    for (VideoStream* s : _streams) {
        if (s->name() == name)
            return s;
    }
    return nullptr;
}

VideoStream* VideoStreamModel::streamForRole(int role) const
{
    // int parameter (not VideoStream::Role) to keep Q_INVOKABLE QML-friendly;
    // QML enum values arrive as int. Bounds-check before casting so a
    // stray/garbage value from QML doesn't produce UB via the enum cast.
    if (role < 0 || role >= VideoStream::roleCount())
        return nullptr;
    const auto target = static_cast<VideoStream::Role>(role);
    for (VideoStream* s : _streams) {
        if (s->role() == target)
            return s;
    }
    return nullptr;
}

int VideoStreamModel::activeCount() const
{
    int count = 0;
    for (const VideoStream* s : _streams) {
        if (s->started())
            ++count;
    }
    return count;
}

void VideoStreamModel::_onStreamChanged(VideoStream* stream, const QList<int>& roles)
{
    const int row = _indexOf(stream);
    if (row >= 0) {
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx, QVector<int>(roles.cbegin(), roles.cend()));
    }
}

int VideoStreamModel::_indexOf(VideoStream* stream) const
{
    return _streams.indexOf(stream);
}
