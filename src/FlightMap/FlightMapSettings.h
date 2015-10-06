/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

#ifndef FlightMapSettings_H
#define FlightMapSettings_H

#include "QGCSingleton.h"

#include <QStringList>

class FlightMapSettings : public QObject
{
    Q_OBJECT
    
    DECLARE_QGC_SINGLETON(FlightMapSettings, FlightMapSettings)

public:
    /// mapProvider is either Bing, Google or Open to specify to set of maps available
    Q_PROPERTY(QString mapProvider READ mapProvider WRITE setMapProvider NOTIFY mapProviderChanged)
    
    /// Map types associated with current map provider
    Q_PROPERTY(QStringList mapTypes MEMBER _mapTypes NOTIFY mapTypesChanged)
    
    Q_INVOKABLE QString mapTypeForMapName(const QString& mapName);
    Q_INVOKABLE void setMapTypeForMapName(const QString& mapName, const QString& mapType);
    
    Q_INVOKABLE void    saveMapSetting (const QString &mapName, const QString& key, const QString& value);
    Q_INVOKABLE QString loadMapSetting (const QString &mapName, const QString& key, const QString& defaultValue);
    
    // Property accessors
    
    QString mapProvider(void);
    void setMapProvider(const QString& mapProvider);
    
signals:
    void mapProviderChanged(const QString& mapProvider);
    void mapTypesChanged(const QStringList& mapTypes);
    
private:
    /// @brief All access to FlightMapSettings singleton is through FlightMapSettings::instance
    FlightMapSettings(QObject* parent = NULL);
    ~FlightMapSettings();
    
    void _storeSettings(void);
    void _loadSettings(void);
    
    void _setMapTypesForCurrentProvider(void);
    
    QString     _mapProvider;               ///< Current map provider
    QStringList _supportedMapProviders;
    QStringList _mapTypes;                  ///< Map types associated with current map provider
    
    static const char* _defaultMapProvider;
    static const char* _settingsGroup;
    static const char* _mapProviderKey;
    static const char* _mapTypeKey;
};

#endif
