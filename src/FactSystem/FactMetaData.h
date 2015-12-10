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

    typedef QVariant (*Translator)(const QVariant& from);
    
    FactMetaData(QObject* parent = NULL);
    FactMetaData(ValueType_t type, QObject* parent = NULL);
    FactMetaData(const FactMetaData& other, QObject* parent = NULL);

    const FactMetaData& operator=(const FactMetaData& other);

    int             decimalPlaces           (void) const { return _decimalPlaces; }
    QVariant        defaultValue            (void) const;
    bool            defaultValueAvailable   (void) const { return _defaultValueAvailable; }
    QStringList     enumStrings             (void) const { return _enumStrings; }
    QVariantList    enumValues              (void) const { return _enumValues; }
    QString         group                   (void) const { return _group; }
    QString         longDescription         (void) const { return _longDescription;}
    QVariant        max                     (void) const { return _max; }
    bool            maxIsDefaultForType     (void) const { return _maxIsDefaultForType; }
    QVariant        min                     (void) const { return _min; }
    bool            minIsDefaultForType     (void) const { return _minIsDefaultForType; }
    QString         name                    (void) const { return _name; }
    QString         shortDescription        (void) const { return _shortDescription; }
    ValueType_t     type                    (void) const { return _type; }
    QString         units                   (void) const { return _units; }

    Translator      rawTranslator           (void) const { return _rawTranslator; }
    Translator      cookedTranslator        (void) const { return _cookedTranslator; }

    void setDecimalPlaces   (int decimalPlaces)                 { _decimalPlaces = decimalPlaces; }
    void setDefaultValue    (const QVariant& defaultValue);
    void setEnumInfo        (const QStringList& strings, const QVariantList& values);
    void setGroup           (const QString& group)              { _group = group; }
    void setLongDescription (const QString& longDescription)    { _longDescription = longDescription;}
    void setMax             (const QVariant& max);
    void setMin             (const QVariant& max);
    void setName            (const QString& name)               { _name = name; }
    void setShortDescription(const QString& shortDescription)   { _shortDescription = shortDescription; }
    void setUnits(const QString& units)                         { _units = units; }
    void setTranslators(Translator rawTranslator, Translator cookedTranslator);
    static QVariant defaultTranslator(const QVariant& from) { return from; }

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
    
    ValueType_t     _type;                  // must be first for correct constructor init
    int             _decimalPlaces;
    QVariant        _defaultValue;
    bool            _defaultValueAvailable;
    QStringList     _enumStrings;
    QVariantList    _enumValues;
    QString         _group;
    QString         _longDescription;
    QVariant        _max;
    bool            _maxIsDefaultForType;
    QVariant        _min;
    bool            _minIsDefaultForType;
    QString         _name;
    QString         _shortDescription;
    QString         _units;
    Translator      _rawTranslator;
    Translator      _cookedTranslator;
};

#endif
