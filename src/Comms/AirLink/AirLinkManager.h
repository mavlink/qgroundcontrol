/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class QNetworkAccessManager;

Q_DECLARE_LOGGING_CATEGORY(AirLinkManagerLog)

class AirLinkManager : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_SINGLETON

    Q_PROPERTY(QStringList droneList READ droneList NOTIFY droneListChanged)

public:
    explicit AirLinkManager(QObject *parent = nullptr);
    ~AirLinkManager();

    /// Gets the singleton instance of AirLinkManager.
    ///     @return The singleton instance.
    static AirLinkManager *instance();

    Q_INVOKABLE void updateDroneList(const QString &login, const QString &pass) { _connectToAirLinkServer(login, pass); }
    Q_INVOKABLE bool isOnline(const QString &drone);
    Q_INVOKABLE void updateCredentials(const QString &login, const QString &pass);

    QStringList droneList() const { return _vehiclesFromServer.keys(); }

signals:
    void droneListChanged();

private slots:
    void _processReplyAirlinkServer();

private:
    void _connectToAirLinkServer(const QString &login, const QString &pass);
    void _parseAnswer(const QByteArray &ba);

    QNetworkAccessManager *_mngr = nullptr;
    QMap<QString, bool> _vehiclesFromServer;
};
