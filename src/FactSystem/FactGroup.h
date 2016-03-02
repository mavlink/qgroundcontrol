/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef FactGroup_H
#define FactGroup_H

#include "Fact.h"
#include "QGCLoggingCategory.h"

#include <QStringList>
#include <QMap>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(VehicleLog)

/// Used to group Facts together into an object hierarachy.
class FactGroup : public QObject
{
    Q_OBJECT
    
public:
    FactGroup(int updateRateMsecs, const QString& metaDataFile, QObject* parent = NULL);

    Q_PROPERTY(QStringList factNames        READ factNames      CONSTANT)
    Q_PROPERTY(QStringList factGroupNames   READ factGroupNames CONSTANT)

    /// @return Fact for specified name, NULL if not found
    Q_INVOKABLE Fact* getFact(const QString& name);

    /// @return FactGroup for specified name, NULL if not found
    Q_INVOKABLE FactGroup* getFactGroup(const QString& name);

    QStringList factNames(void) const { return _nameToFactMap.keys(); }
    QStringList factGroupNames(void) const { return _nameToFactGroupMap.keys(); }
    
protected:
    void _addFact(Fact* fact, const QString& name);
    void _addFactGroup(FactGroup* factGroup, const QString& name);

    int _updateRateMSecs;   ///< Update rate for Fact::valueChanged signals, 0: immediate update

private slots:
    void _updateAllValues(void);

private:
    void _loadMetaData(const QString& filename);

    QMap<QString, Fact*>            _nameToFactMap;
    QMap<QString, FactGroup*>       _nameToFactGroupMap;
    QMap<QString, FactMetaData*>    _nameToFactMetaDataMap;

    QTimer _updateTimer;

    static const char*  _propertiesJsonKey;
    static const char*  _nameJsonKey;
    static const char*  _decimalPlacesJsonKey;
    static const char*  _typeJsonKey;
    static const char*  _versionJsonKey;
    static const char*  _shortDescriptionJsonKey;
    static const char*  _unitsJsonKey;
    static const char*  _defaultValueJsonKey;
    static const char*  _minJsonKey;
    static const char*  _maxJsonKey;
};

#endif
