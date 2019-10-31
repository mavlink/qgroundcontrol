#ifndef VIDEORECEIVERMODEL_H
#define VIDEORECEIVERMODEL_H

#include <QAbstractListModel>

class VideoReceiver;

/* This class controls the creation, deletion and displaying of the
 * VideoReceivers in the Qml.
 */
class VideoReceiverModel : public QAbstractListModel
{
    Q_OBJECT
public:
    VideoReceiverModel();
    Q_INVOKABLE void createVideoStream();
    Q_INVOKABLE void deleteVideoStream(int pos);

    // overrides
    QVariant data(const QModelIndex &idx, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

private:
    QList<VideoReceiver*> m_videoReceivers;
};

#endif // VideoReceiverModel
