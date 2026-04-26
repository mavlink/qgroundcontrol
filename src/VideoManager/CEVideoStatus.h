/****************************************************************************
 *
 * CrownEagle video status receiver.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtNetwork/QUdpSocket>

class CEVideoStatus : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY statusChanged)
    Q_PROPERTY(double fps READ fps NOTIFY statusChanged)
    Q_PROPERTY(double lastFrameAgeSec READ lastFrameAgeSec NOTIFY statusChanged)
    Q_PROPERTY(bool stale READ stale NOTIFY statusChanged)
    Q_PROPERTY(bool hasStatus READ hasStatus NOTIFY statusChanged)

public:
    explicit CEVideoStatus(QObject *parent = nullptr);

    QString state() const { return _state; }
    double fps() const { return _fps; }
    double lastFrameAgeSec() const { return _lastFrameAgeSec; }
    bool stale() const { return _state == QStringLiteral("STALE"); }
    bool hasStatus() const { return _hasStatus; }

signals:
    void statusChanged();

private slots:
    void _readPendingDatagrams();
    void _checkTimeout();

private:
    void _setStatus(const QString &state, double fps, double lastFrameAgeSec, bool hasStatus);

    QUdpSocket _socket;
    QTimer _timeoutTimer;
    QElapsedTimer _lastPacketTimer;
    QString _state = QStringLiteral("NO_STATUS");
    double _fps = 0.0;
    double _lastFrameAgeSec = -1.0;
    bool _hasStatus = false;
};
