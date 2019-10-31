#include "VideoReceiverModel.h"
#include "VideoReceiver.h"

#include <QSettings>
#include <QUuid>

/* Adds a UUID to the Video, so it's easy to store and retreieve setttings for it.
* This is not using the Fact system as I did not understand how to use it for
* Array types
*
* Hm, maybe this should just manage the settings. let's see.
*/
VideoReceiverModel::VideoReceiverModel()
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("VideoManagement"));
    const auto childGroups = settings.childGroups();
    for (const auto& videoReceiverSettings : childGroups) {
        settings.beginGroup(videoReceiverSettings);

        //TODO: Read the video stream settings.
        auto videoReceiver = new VideoReceiver();
        videoReceiver->setProperty("uuid", videoReceiverSettings.split('_').at(1));
        m_videoReceivers.append(videoReceiver);
    }
}

void VideoReceiverModel::createVideoStream()
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    auto videoReceiver = new VideoReceiver();
    m_videoReceivers.append(videoReceiver);

    QSettings settings;
    QUuid uuid = QUuid::createUuid();
    settings.beginGroup(QStringLiteral("VideoManagement"));
    settings.beginGroup(QStringLiteral("VideoStream_%1").arg(uuid.toString()));
    // TODO: Store the defaults for the Video Stream.
    videoReceiver->setProperty("uuid", uuid.toString());
    endInsertRows();
}

void VideoReceiverModel::deleteVideoStream(int pos)
{
    beginRemoveRows(QModelIndex(), pos, pos);
    auto *toBeDeleted = m_videoReceivers.takeAt(pos);
    QString uuid = toBeDeleted->property("uuid").toString();

    QSettings settings;
    settings.beginGroup(QStringLiteral("VideoManagement"));
        settings.beginGroup(QStringLiteral("VideoStream_%1").arg(uuid));
            settings.remove(""); // erase group.
        settings.endGroup();
    settings.endGroup();
    toBeDeleted->deleteLater();
    endRemoveRows();
}

// overrides
QVariant VideoReceiverModel::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid() || idx.row() >= m_videoReceivers.count() || role != Qt::UserRole ) {
        return {};
    }

    return QVariant::fromValue<VideoReceiver*>(m_videoReceivers.at(idx.row()));
}

QHash<int, QByteArray> VideoReceiverModel::roleNames() const
{
    return { {Qt::UserRole, "videoReceiver"} };
}

int VideoReceiverModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return m_videoReceivers.count();
}
