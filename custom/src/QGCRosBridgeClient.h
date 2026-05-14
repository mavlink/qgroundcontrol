#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtNetwork/QHostAddress>

class QUdpSocket;

class QGCRosBridgeClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)

public:
    explicit QGCRosBridgeClient(QObject *parent = nullptr);

    QString host() const;
    void setHost(const QString &host);

    int port() const;
    void setPort(int port);

    // QGC QML -> Qt UDP client -> ROS 2 qgc_ros_bridge_node -> ROS 2 topics.
    Q_INVOKABLE bool sendJsonMessage(const QVariantMap &message);

signals:
    void hostChanged();
    void portChanged();
    void messageSent(const QString &json);
    void sendFailed(const QString &reason);

private:
    QUdpSocket *_socket = nullptr;
    QString _host = QStringLiteral("127.0.0.1");
    quint16 _port = 5010;
};
