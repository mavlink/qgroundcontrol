/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QSettings>
#include <QtCore/QString>

class LinkInterface;

/// Interface holding link specific settings.
class LinkConfiguration : public QObject
{
    Q_OBJECT
    Q_MOC_INCLUDE("LinkInterface.h")

    Q_PROPERTY(QString          name            READ name           WRITE setName           NOTIFY nameChanged)
    Q_PROPERTY(LinkInterface    *link           READ link                                   NOTIFY linkChanged)
    Q_PROPERTY(LinkType         linkType        READ type                                   CONSTANT)
    Q_PROPERTY(bool             dynamic         READ isDynamic      WRITE setDynamic        NOTIFY dynamicChanged)
    Q_PROPERTY(bool             autoConnect     READ isAutoConnect  WRITE setAutoConnect    NOTIFY autoConnectChanged)
    Q_PROPERTY(QString          settingsURL     READ settingsURL                            CONSTANT)
    Q_PROPERTY(QString          settingsTitle   READ settingsTitle                          CONSTANT)
    Q_PROPERTY(bool             highLatency     READ isHighLatency  WRITE setHighLatency    NOTIFY highLatencyChanged)

public:
    LinkConfiguration(const QString &name, QObject *parent = nullptr);
    LinkConfiguration(const LinkConfiguration *copy, QObject *parent = nullptr);
    virtual ~LinkConfiguration();

    QString name() const { return _name; }
    void setName(const QString &name);

    LinkInterface *link() const { return _link.lock().get(); }
    void setLink(const std::shared_ptr<LinkInterface> link);

    /// Is this a dynamic configuration?
    ///     @return True if not persisted
    bool isDynamic() const { return _dynamic; }

    /// Set if this is this a dynamic configuration. (decided at runtime)
    void setDynamic(bool dynamic = true);

    /// Is this a forwarding link configuration?
    ///     @return True if forwarding
    bool isForwarding() const { return _forwarding; }

    /// Set if this is this a forwarding link configuration. (decided at runtime)
    void setForwarding(bool forwarding = true) { _forwarding = forwarding; };

    bool isAutoConnect() const { return _autoConnect; }

    /// Set if this is this an Auto Connect configuration.
    virtual void setAutoConnect(bool autoc = true);

    /// Is this a High Latency configuration?
    ///     @return True if this is an High Latency configuration (link with large delays).
    bool isHighLatency() const { return _highLatency; }

    /// Set if this is this an High Latency configuration.
    void setHighLatency(bool hl = false);

    /// Copy instance data, When manipulating data, you create a copy of the configuration using the copy constructor,
    /// edit it and then transfer its content to the original using this method.
    ///     @param[in] source The source instance (the edited copy)
    virtual void copyFrom(const LinkConfiguration *source);

    /// The link types supported by QGC
    /// Any changes here MUST be reflected in LinkManager::linkTypeStrings()
    enum LinkType {
#ifndef QGC_NO_SERIAL_LINK
        TypeSerial,     ///< Serial Link
#endif
        TypeUdp,        ///< UDP Link
        TypeTcp,        ///< TCP Link
#ifdef QGC_ENABLE_BLUETOOTH
        TypeBluetooth,  ///< Bluetooth Link
#endif
#ifdef QT_DEBUG
        TypeMock,       ///< Mock Link for Unitesting
#endif
#ifndef QGC_AIRLINK_DISABLED
        AirLink,
#endif
        TypeLogReplay,
        TypeLast        // Last type value (type >= TypeLast == invalid)
    };
    Q_ENUM(LinkType)

    /// Connection type, pure virtual method returning one of the -TypeXxx types above.
    ///     @return The type of links these settings belong to.
    virtual LinkType type() const = 0;

    /// Load settings, Pure virtual method telling the instance to load its configuration.
    ///     @param[in] settings The QSettings instance to use
    ///     @param[in] root The root path of the setting.
    virtual void loadSettings(QSettings &settings, const QString &root) = 0;

    /// Save settings, Pure virtual method telling the instance to save its configuration.
    ///     @param[in] settings The QSettings instance to use
    ///     @param[in] root The root path of the setting.
    virtual void saveSettings(QSettings &settings, const QString &root) const = 0;

    /// Settings URL, Pure virtual method providing the URL for the (QML) settings dialog
    virtual QString settingsURL() const = 0;

    /// Settings Title, Pure virtual method providing the Title for the (QML) settings dialog
    virtual QString settingsTitle() const = 0;

    /// Configuration Factory to create new link configuration instance based on the given type.
    ///     @return A new instance of the given type
    static LinkConfiguration *createSettings(int type, const QString &name);

    /// Duplicate configuration instance. Helper method to create a new instance copy for editing.
    ///     @return A new copy of the given settings instance
    static LinkConfiguration *duplicateSettings(const LinkConfiguration *source);

    /// Root path for QSettings
    ///     @return The root path of the settings.
    static QString settingsRoot() { return QStringLiteral("LinkConfigurations"); }

signals:
    void nameChanged(const QString &name);
    void linkChanged();
    void dynamicChanged();
    void autoConnectChanged();
    void highLatencyChanged();

protected:
    std::weak_ptr<LinkInterface> _link; ///< Link currently using this configuration (if any)

private:
    QString _name;
    bool _dynamic = false;     ///< A connection added automatically and not persistent (unless it's edited).
    bool _forwarding = false;  ///< Automatically added Mavlink forwarding connection
    bool _autoConnect = false; ///< This connection is started automatically at boot
    bool _highLatency = false;
};

typedef std::shared_ptr<LinkConfiguration> SharedLinkConfigurationPtr;
typedef std::weak_ptr<LinkConfiguration> WeakLinkConfigurationPtr;
