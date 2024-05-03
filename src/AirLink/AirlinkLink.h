#pragma once

#include <QtCore/QMutex>

#include "UDPLink.h"

class AirlinkConfiguration : public UDPConfiguration
{
    Q_OBJECT
public:
    Q_PROPERTY(QString username     READ username       WRITE setUsername   NOTIFY usernameChanged)
    Q_PROPERTY(QString password     READ password       WRITE setPassword   NOTIFY passwordChanged)
    Q_PROPERTY(QString modemName    READ modemName      WRITE setModemName  NOTIFY modemNameChanged)

    AirlinkConfiguration(const QString& name);
    AirlinkConfiguration(AirlinkConfiguration* source);
    ~AirlinkConfiguration();

    QString username () const { return _username; }
    QString password () const { return _password; }
    QString modemName() const { return _modemName; }

    void setUsername    (QString username);
    void setPassword    (QString password);
    void setModemName   (QString modemName);

    /// LinkConfiguration overrides
    LinkType    type                 (void) override { return LinkConfiguration::Airlink; }
    void        loadSettings         (QSettings& settings, const QString& root) override;
    void        saveSettings         (QSettings& settings, const QString& root) override;
    QString     settingsURL          (void) override { return "AirLinkSettings.qml"; }
    QString     settingsTitle        (void) override { return tr("Airlink Link Settings"); }
    void        copyFrom             (LinkConfiguration* source) override;


signals:
    void usernameChanged    (void);
    void passwordChanged    (void);
    void modemNameChanged   (void);

private:
    void _copyFrom          (LinkConfiguration *source);


    QString _username;
    QString _password;
    QString _modemName;

    const QString _usernameSettingsKey = "username";
    const QString _passwordSettingsKey = "password";
    const QString _modemNameSettingsKey = "modemName";
};

class AirlinkLink : public UDPLink
{
    Q_OBJECT
public:
    AirlinkLink(SharedLinkConfigurationPtr& config);
    virtual ~AirlinkLink();

    /// LinkInterface overrides
    // bool isConnected(void) const override;
    void disconnect (void) override;

    /// QThread overrides
    // void run(void) override;

private:
    /// LinkInterface overrides
    bool _connect(void) override;

    void _configureUdpSettings();
    void _sendLoginMsgToAirLink();
    bool _stillConnecting();
    void _setConnectFlag(bool connect);

    QMutex _mutex;
    /// Access this varible only with _mutex locked
    bool _needToConnect {false};
};
