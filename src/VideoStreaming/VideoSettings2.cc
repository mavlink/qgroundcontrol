#include "VideoSettings2.h"
#include "VideoReceiver.h"
#include <QSettings>
#include <QUuid>

VideoSettings2::VideoSettings2()
{

}

Q_INVOKABLE QStringList VideoSettings2::streamTypes() const
{
    return QStringList {
        "No Video Available",
        "Video Stream Disabled",
#ifdef QGC_GST_STREAMING
        "RTSP Video Stream",
#ifndef NO_UDP_VIDEO
        "UDP h.264 Video Stream",
        "UDP h.265 Video Stream",
#endif
        "TCP-MPEG2 Video Stream",
        "MPEG-TS (h.264) Video Stream"
#endif
    };
}

QString VideoSettings2::uuid() const
{
    return _uuid;
}
void VideoSettings2::setUuid(const QString& value)
{
    if (_uuid != value) {
        _uuid = value;
        emit uuidChanged();
    }
}

QUrl VideoSettings2::host() const
{
    return _host;
}

void VideoSettings2::setHost(const QUrl& value)
{
    if (_host != value) {
        _host = value;
        emit hostChanged();
    }
}

int VideoSettings2::port() const
{
    return _port;
}

void VideoSettings2::setPort(int value)
{
    if (_port != value) {
        _port = value;
        emit portChanged();
    }
}

QString VideoSettings2::streamType() const
{
    return _streamType;
}

void VideoSettings2::setStreamType(const QString& value)
{
    if (_streamType != value) {
        if(streamTypes().contains(value)) {
            _streamType = value;
            emit streamTypeChanged();
        }
    }
}

QString VideoSettings2::caps() const
{
    return _caps;
}

void VideoSettings2::setCaps(const QString& value)
{
    if (_caps != value) {
        _caps = value;
        emit capsChanged();
    }
}

QString VideoSettings2::name() const
{
    return _name;
}

Q_SLOT void VideoSettings2::setName(const QString& value)
{
    if (_name != value) {
        _name = value;
        emit nameChanged();
    }
}

QString VideoSettings2::createNewEntry()
{
    QSettings settings;
    QUuid uuid = QUuid::createUuid();
    settings.beginGroup(QStringLiteral("VideoManagement"));
    settings.beginGroup(QStringLiteral("VideoStream_%1").arg(uuid.toString()));
    settings.setValue("name", tr("Untitled"));
    _uuid = uuid.toString();
}

void VideoSettings2::removeEntry()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("VideoManagement"));
        settings.beginGroup(QStringLiteral("VideoStream_%1").arg(_uuid));
            settings.remove(""); // erase group.
        settings.endGroup();
    settings.endGroup();

}
Q_INVOKABLE void VideoSettings2::save()
{
    //TODO: Move the UUIO to be a real property.
    if (!_uuid.size()) {
        return;
    }

    QSettings settings;
    settings.beginGroup(QStringLiteral("VideoManagement"));
        settings.beginGroup(QStringLiteral("VideoStream_%1").arg(_uuid));
            settings.setValue("name", _name);
            settings.setValue("caps", _caps);
            settings.setValue("streamType", _streamType);
            settings.setValue("port", _port);
            settings.setValue("host", _host);
            settings.setValue("pattern", 10);
        settings.endGroup();
    settings.endGroup();

    emit settingsChanged();
}

Q_INVOKABLE void VideoSettings2::cancel()
{
    _uuid.clear();
}
