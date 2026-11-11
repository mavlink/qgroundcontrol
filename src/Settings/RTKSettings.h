#pragma once

#include <QObject>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtQmlIntegration/QtQmlIntegration>

#include "SettingsGroup.h"

class BaseModeDefinition : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

public:
    enum class Mode
    {
        BaseSurveyIn = 0,
        BaseFixed = 1,
        BaseReceiverAveraging = 2,
    };
    Q_ENUM(Mode)

private:
    explicit BaseModeDefinition(QObject* parent = nullptr) : QObject(parent) {}
};

/// A base position saved under a name, which a fixed-position base can start from.
struct RTKBaseSite
{
    Q_GADGET
    QML_VALUE_TYPE(rtkBaseSite)
    Q_PROPERTY(QString name MEMBER name FINAL)
    Q_PROPERTY(double latitude MEMBER latitude FINAL)
    Q_PROPERTY(double longitude MEMBER longitude FINAL)
    Q_PROPERTY(double altitude MEMBER altitude FINAL)
    Q_PROPERTY(double accuracy MEMBER accuracy FINAL)

public:
    QString name;
    /// WGS84 degrees.
    double latitude = 0.;
    double longitude = 0.;
    /// Meters above the WGS84 ellipsoid, as fixedBasePositionAltitude.
    double altitude = 0.;
    /// Meters.
    double accuracy = 0.;

    bool operator==(const RTKBaseSite&) const = default;
};

class RTKSettings : public SettingsGroup
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
    /// The saved base sites, in the order they were first saved. The fixed base position Facts stay the position a
    /// receiver is configured from; useBaseSite() copies a site into them.
    Q_PROPERTY(QList<RTKBaseSite> baseSites READ baseSites NOTIFY baseSitesChanged FINAL)

public:
    RTKSettings(QObject* parent = nullptr);

    /// Persisted values of receiverRole. The storage type keeps any saved value a valid enum value.
    enum ReceiverRole : quint8
    {
        Passive = 1,
        ConfiguredBase = 2,
    };
    Q_ENUM(ReceiverRole)

    /// Persisted values of connectionType. The storage type keeps any saved value a valid enum value.
    enum ConnectionType : quint8
    {
        Serial = 0,
        Tcp = 1,
        Udp = 2,
    };
    Q_ENUM(ConnectionType)

    /// Limits of the saved base sites, as an enum so that QML reads them from the type.
    enum BaseSiteLimit : int
    {
        MaxBaseSites = 50,
        MaxBaseSiteNameLength = 64,
    };
    Q_ENUM(BaseSiteLimit)

    QList<RTKBaseSite> baseSites() const { return _baseSites; }

    /// Saves @a site under its name without surrounding spaces, replacing the site of that name. Returns false,
    /// saving nothing, for a name that is empty or longer than MaxBaseSiteNameLength, a position off the globe or not
    /// finite, or a new name when MaxBaseSites sites are saved.
    bool saveBaseSite(const RTKBaseSite& site);
    /// Saves the fixed base position under @a siteName, as saveBaseSite() does.
    Q_INVOKABLE bool saveFixedBasePositionAsSite(const QString& siteName);
    /// Makes the site saved under @a siteName the fixed base position. Returns false when no site has that name.
    Q_INVOKABLE bool useBaseSite(const QString& siteName);
    /// Returns false when no site has that name.
    Q_INVOKABLE bool removeBaseSite(const QString& siteName);

    DEFINE_SETTING_NAME_GROUP()
    DEFINE_SETTINGFACT(receiverRole)
    DEFINE_SETTINGFACT(forwardReceiverRtcm)
    DEFINE_SETTINGFACT(baseReceiverManufacturers)
    DEFINE_SETTINGFACT(surveyInAccuracyLimit)
    DEFINE_SETTINGFACT(surveyInMinObservationDuration)
    DEFINE_SETTINGFACT(receiverAveragingDuration)
    DEFINE_SETTINGFACT(connectionType)
    DEFINE_SETTINGFACT(tcpHost)
    DEFINE_SETTINGFACT(tcpPort)
    DEFINE_SETTINGFACT(udpPort)
    DEFINE_SETTINGFACT(autoConnect)
    DEFINE_SETTINGFACT(gcsPositionSource)
    DEFINE_SETTINGFACT(serialDevice)
    DEFINE_SETTINGFACT(serialBaudRate)
    DEFINE_SETTINGFACT(useFixedBasePosition)
    DEFINE_SETTINGFACT(fixedBasePositionLatitude)
    DEFINE_SETTINGFACT(fixedBasePositionLongitude)
    DEFINE_SETTINGFACT(fixedBasePositionAltitude)
    DEFINE_SETTINGFACT(fixedBasePositionAccuracy)
    DEFINE_SETTINGFACT(compactRtcmCorrections)
    DEFINE_SETTINGFACT(savedBaseSites)

signals:
    void baseSitesChanged();

private:
    /// Reads the sites from savedBaseSites, which holds them as JSON, dropping the entries that are not valid.
    void _loadBaseSites();
    void _storeBaseSites(const QList<RTKBaseSite>& sites);

    QList<RTKBaseSite> _baseSites;
};
