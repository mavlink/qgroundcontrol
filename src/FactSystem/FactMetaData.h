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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FactMetaData_H
#define FactMetaData_H

#include <QObject>
#include <QString>
#include <QVariant>

/// Holds the meta data associated with a Fact.
///
/// Holds the meta data associated with a Fact. This is kept in a seperate object from the Fact itself
/// since you may have multiple instances of the same Fact. But there is only ever one FactMetaData
/// instance or each Fact.
class FactMetaData : public QObject
{
    Q_OBJECT
    
public:
    typedef enum {
        valueTypeUint8,
        valueTypeInt8,
        valueTypeUint16,
        valueTypeInt16,
        valueTypeUint32,
        valueTypeInt32,
        valueTypeFloat,
        valueTypeDouble
    } ValueType_t;
    
    FactMetaData(QObject* parent = NULL);
    FactMetaData(ValueType_t type, QObject* parent = NULL);
    FactMetaData(const FactMetaData& other, QObject* parent = NULL);

    const FactMetaData& operator=(const FactMetaData& other);

    // Property accessors
    QString     name(void) const                    { return _name; }
    QString     group(void) const                   { return _group; }
    ValueType_t type(void) const                    { return _type; }
    QVariant    defaultValue(void) const;
    bool		defaultValueAvailable(void) const   { return _defaultValueAvailable; }
    QString     shortDescription(void) const        { return _shortDescription; }
    QString     longDescription(void) const         { return _longDescription;}
    QString     units(void) const                   { return _units; }
    QVariant    min(void) const                     { return _min; }
    QVariant    max(void) const                     { return _max; }
    bool        minIsDefaultForType(void) const     { return _minIsDefaultForType; }
    bool        maxIsDefaultForType(void) const     { return _maxIsDefaultForType; }
    int         decimalPlaces(void) const           { return _decimalPlaces; }

    // Property setters
    void setName(const QString& name)                           { _name = name; }
    void setGroup(const QString& group)                         { _group = group; }
    void setDefaultValue(const QVariant& defaultValue);
    void setShortDescription(const QString& shortDescription)   { _shortDescription = shortDescription; }
    void setLongDescription(const QString& longDescription)     { _longDescription = longDescription;}
    void setUnits(const QString& units)                         { _units = units; }
    void setMin(const QVariant& max);
    void setMax(const QVariant& max);
    void setDecimalPlaces(int decimalPlaces)                    { _decimalPlaces = decimalPlaces; }
    
    /// Converts the specified value, validating against meta data
    ///     @param value Value to convert, can be string
    ///     @param convertOnly true: convert to correct type only, do not validate against meta data
    ///     @param typeValue Converted value, correctly typed
    ///     @param errorString Error string if convert fails
    /// @returns false: Convert failed, errorString set
    bool convertAndValidate(const QVariant& value, bool convertOnly, QVariant& typedValue, QString& errorString);

    static const int defaultDecimalPlaces = 3;

private:
    QVariant _minForType(void) const;
    QVariant _maxForType(void) const;
    
    QString     _name;
    QString     _group;
    ValueType_t _type;
    QVariant    _defaultValue;
	bool		_defaultValueAvailable;
    QString     _shortDescription;
    QString     _longDescription;
    QString     _units;
    QVariant    _min;
    QVariant    _max;
    bool        _minIsDefaultForType;
    bool        _maxIsDefaultForType;
    int         _decimalPlaces;
};

#endif
