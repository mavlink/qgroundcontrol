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
#include <QtCore/QMutex>

#include "UDPLink.h"

Q_DECLARE_LOGGING_CATEGORY(AirLinkLinkLog)

class AirLinkConfiguration : public UDPConfiguration
{
    Q_OBJECT

    Q_PROPERTY(QString username     READ username   WRITE setUsername   NOTIFY usernameChanged)
    Q_PROPERTY(QString password     READ password   WRITE setPassword   NOTIFY passwordChanged)
    Q_PROPERTY(QString modemName    READ modemName  WRITE setModemName  NOTIFY modemNameChanged)

public:
    explicit AirLinkConfiguration(const QString &name, QObject *parent = nullptr);
    explicit AirLinkConfiguration(const AirLinkConfiguration *copy, QObject *parent = nullptr);
    ~AirLinkConfiguration();

    QString username() const { return _username; }
    QString password() const { return _password; }
    QString modemName() const { return _modemName; }

    void setUsername(const QString &username);
    void setPassword(const QString &password);
    void setModemName(const QString &modemName);

    LinkType type() const final { return LinkConfiguration::AirLink; }
    void copyFrom(const LinkConfiguration *source) final;

    void loadSettings(QSettings &settings, const QString &root) final;
    void saveSettings(QSettings &settings, const QString &root) const final;
    QString settingsURL() const final { return "AirLinkSettings.qml"; }
    QString settingsTitle() const final { return tr("AirLink Link Settings"); }

signals:
    void usernameChanged();
    void passwordChanged();
    void modemNameChanged();

private:
    QString _username;
    QString _password;
    QString _modemName;

    const QString _usernameSettingsKey = QStringLiteral("username");
    const QString _passwordSettingsKey = QStringLiteral("password");
    const QString _modemNameSettingsKey = QStringLiteral("modemName");
};

/*===========================================================================*/

class AirLinkLink : public UDPLink
{
    Q_OBJECT

public:
    AirLinkLink(SharedLinkConfigurationPtr &config, QObject *parent = nullptr);
    ~AirLinkLink();

    void disconnect() final;

private slots:
    bool _connect() final;

private:
    void _configureUdpSettings();
    void _sendLoginMsgToAirLink();
    bool _stillConnecting();
    void _setConnectFlag(bool connect);

    const AirLinkConfiguration *_AirLinkConfig = nullptr;
    QMutex _mutex;
    bool _needToConnect = false;

    static constexpr const char *_airLinkHost = "air-link.space";
    static constexpr int _airLinkPort = 10000;
};
