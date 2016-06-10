/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef FlightMapSettings_H
#define FlightMapSettings_H

#include "QGCToolbox.h"

#include <QObject>
#include <QStringList>

class FlightMapSettings : public QGCTool
{
    Q_OBJECT

public:
    FlightMapSettings(QGCApplication* app);

    /// mapProvider is either Bing, Google or Open to specify to set of maps available
    Q_PROPERTY(QString      mapProvider     READ mapProvider    WRITE setMapProvider    NOTIFY mapProviderChanged)

    /// Map providers
    Q_PROPERTY(QStringList  mapProviders    READ mapProviders                           CONSTANT)

    /// Map types associated with current map provider
    Q_PROPERTY(QStringList  mapTypes        MEMBER _mapTypes                            NOTIFY mapTypesChanged)

    /// Map type to be used for all maps
    Q_PROPERTY(QString      mapType         READ mapType        WRITE setMapType        NOTIFY mapTypeChanged)

    Q_INVOKABLE void        saveMapSetting      (const QString &mapName, const QString& key, const QString& value);
    Q_INVOKABLE QString     loadMapSetting      (const QString &mapName, const QString& key, const QString& defaultValue);
    Q_INVOKABLE void        saveBoolMapSetting  (const QString &mapName, const QString& key, bool value);
    Q_INVOKABLE bool        loadBoolMapSetting  (const QString &mapName, const QString& key, bool defaultValue);

    // Property accessors

    QString mapProvider(void);
    void setMapProvider(const QString& mapProvider);

    QString mapType(void);
    void setMapType(const QString& mapType);

    // Override from QGCTool
    virtual void setToolbox(QGCToolbox *toolbox);

    QStringList mapProviders() { return _supportedMapProviders; }

signals:
    void mapProviderChanged (const QString& mapProvider);
    void mapTypesChanged    (const QStringList& mapTypes);
    void mapTypeChanged     (const QString& mapType);

private:
    void    _storeSettings              (void);
    void    _loadSettings               (void);

    void    _setMapTypesForCurrentProvider(void);

    QString     _mapProvider;               ///< Current map provider
    QStringList _supportedMapProviders;
    QStringList _mapTypes;                  ///< Map types associated with current map provider

    static const char* _defaultMapProvider;
    static const char* _settingsGroup;
    static const char* _mapProviderKey;
    static const char* _mapTypeKey;
};

#endif
