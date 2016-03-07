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

#ifndef Fact_H
#define Fact_H

#include "FactMetaData.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>

/// @brief A Fact is used to hold a single value within the system.
class Fact : public QObject
{
    Q_OBJECT
    
public:
    Fact(QObject* parent = NULL);
    Fact(int componentId, QString name, FactMetaData::ValueType_t type, QObject* parent = NULL);
    Fact(const Fact& other, QObject* parent = NULL);

    const Fact& operator=(const Fact& other);

    Q_PROPERTY(int          componentId             READ componentId                                        CONSTANT)
    Q_PROPERTY(QStringList  bitmaskStrings          READ bitmaskStrings                                     NOTIFY bitmaskStringsChanged)
    Q_PROPERTY(QVariantList bitmaskValues           READ bitmaskValues                                      NOTIFY bitmaskValuesChanged)
    Q_PROPERTY(int          decimalPlaces           READ decimalPlaces                                      CONSTANT)
    Q_PROPERTY(QVariant     defaultValue            READ cookedDefaultValue                                 CONSTANT)
    Q_PROPERTY(QString      defaultValueString      READ cookedDefaultValueString                           CONSTANT)
    Q_PROPERTY(bool         defaultValueAvailable   READ defaultValueAvailable                              CONSTANT)
    Q_PROPERTY(int          enumIndex               READ enumIndex              WRITE setEnumIndex          NOTIFY valueChanged)
    Q_PROPERTY(QStringList  enumStrings             READ enumStrings                                        NOTIFY enumStringsChanged)
    Q_PROPERTY(QString      enumStringValue         READ enumStringValue        WRITE setEnumStringValue    NOTIFY valueChanged)
    Q_PROPERTY(QVariantList enumValues              READ enumValues                                         NOTIFY enumValuesChanged)
    Q_PROPERTY(QString      group                   READ group                                              CONSTANT)
    Q_PROPERTY(QString      longDescription         READ longDescription                                    CONSTANT)
    Q_PROPERTY(QVariant     max                     READ cookedMax                                          CONSTANT)
    Q_PROPERTY(QString      maxString               READ cookedMaxString                                    CONSTANT)
    Q_PROPERTY(bool         maxIsDefaultForType     READ maxIsDefaultForType                                CONSTANT)
    Q_PROPERTY(QVariant     min                     READ cookedMin                                          CONSTANT)
    Q_PROPERTY(QString      minString               READ cookedMinString                                    CONSTANT)
    Q_PROPERTY(bool         minIsDefaultForType     READ minIsDefaultForType                                CONSTANT)
    Q_PROPERTY(QString      name                    READ name                                               CONSTANT)
    Q_PROPERTY(bool         rebootRequired          READ rebootRequired                                     CONSTANT)
    Q_PROPERTY(QString      shortDescription        READ shortDescription                                   CONSTANT)
    Q_PROPERTY(FactMetaData::ValueType_t type       READ type                                               CONSTANT)
    Q_PROPERTY(QString      units                   READ cookedUnits                                        CONSTANT)
    Q_PROPERTY(QVariant     value                   READ cookedValue            WRITE setCookedValue        NOTIFY valueChanged)
    Q_PROPERTY(bool         valueEqualsDefault      READ valueEqualsDefault                                 NOTIFY valueChanged)
    Q_PROPERTY(QString      valueString             READ cookedValueString                                  NOTIFY valueChanged)
    Q_PROPERTY(QString      enumOrValueString       READ enumOrValueString                                  NOTIFY valueChanged)
    Q_PROPERTY(double       increment               READ increment                                          CONSTANT)

    /// Convert and validate value
    ///     @param convertOnly true: validate type conversion only, false: validate against meta data as well
    Q_INVOKABLE QString validate(const QString& cookedValue, bool convertOnly);

    QVariant        cookedValue             (void) const;   /// Value after translation
    QVariant        rawValue                (void) const { return _rawValue; }  /// value prior to translation, careful
    int             componentId             (void) const;
    int             decimalPlaces           (void) const;
    QVariant        rawDefaultValue         (void) const;
    QVariant        cookedDefaultValue      (void) const;
    bool            defaultValueAvailable   (void) const;
    QString         cookedDefaultValueString(void) const;
    QStringList     bitmaskStrings          (void) const;
    QVariantList    bitmaskValues           (void) const;
    int             enumIndex               (void);         // This is not const, since an unknown value can modify the enum lists
    QStringList     enumStrings             (void) const;
    QString         enumStringValue         (void);         // This is not const, since an unknown value can modify the enum lists
    QVariantList    enumValues              (void) const;
    QString         group                   (void) const;
    QString         longDescription         (void) const;
    QVariant        rawMax                  (void) const;
    QVariant        cookedMax               (void) const;
    QString         cookedMaxString         (void) const;
    bool            maxIsDefaultForType     (void) const;
    QVariant        rawMin                  (void) const;
    QVariant        cookedMin               (void) const;
    QString         cookedMinString         (void) const;
    bool            minIsDefaultForType     (void) const;
    QString         name                    (void) const;
    QString         shortDescription        (void) const;
    FactMetaData::ValueType_t type          (void) const;
    QString         cookedUnits             (void) const;
    QString         rawUnits                (void) const;
    QString         rawValueString          (void) const;
    QString         cookedValueString       (void) const;
    bool            valueEqualsDefault      (void) const;
    bool            rebootRequired          (void) const;
    QString         enumOrValueString       (void);         // This is not const, since an unknown value can modify the enum lists
    double          increment               (void) const;

    /// Returns the values as a string with full 18 digit precision if float/double.
    QString rawValueStringFullPrecision(void) const;

    void setRawValue        (const QVariant& value);
    void setCookedValue     (const QVariant& value);
    void setEnumIndex       (int index);
    void setEnumStringValue (const QString& value);

    // The following methods allow you to defer sending of the valueChanged signals in order to implement
    // rate limited signalling for ui performance. Used by FactGroup for example.

    void setSendValueChangedSignals (bool sendValueChangedSignals);
    bool sendValueChangedSignals (void) const { return _sendValueChangedSignals; }
    bool deferredValueChangeSignal(void) const { return _deferredValueChangeSignal; }
    void clearDeferredValueChangeSignal(void) { _deferredValueChangeSignal = false; }
    void sendDeferredValueChangedSignal(void);

    // C++ methods

    /// Sets and sends new value to vehicle even if value is the same
    void forceSetRawValue(const QVariant& value);
    
    /// Sets the meta data associated with the Fact.
    void setMetaData(FactMetaData* metaData);
    
    void _containerSetRawValue(const QVariant& value);
    
    /// Generally you should not change the name of a fact. But if you know what you are doing, you can.
    void _setName(const QString& name) { _name = name; }

    
signals:
    void bitmaskStringsChanged(void);
    void bitmaskValuesChanged(void);
    void enumStringsChanged(void);
    void enumValuesChanged(void);
    void sendValueChangedSignalsChanged(bool sendValueChangedSignals);

    /// QObject Property System signal for value property changes
    ///
    /// This signal is only meant for use by the QT property system. It should not be connected to by client code.
    void valueChanged(QVariant value);
    
    /// Signalled when the param write ack comes back from the vehicle
    void vehicleUpdated(QVariant value);
    
    /// Signalled when property has been changed by a call to the property write accessor
    ///
    /// This signal is meant for use by Fact container implementations.
    void _containerRawValueChanged(const QVariant& value);
    
protected:
    QString _variantToString(const QVariant& variant, int decimalPlaces) const;
    void _sendValueChangedSignal(QVariant value);

    QString                     _name;
    int                         _componentId;
    QVariant                    _rawValue;
    FactMetaData::ValueType_t   _type;
    FactMetaData*               _metaData;
    bool                        _sendValueChangedSignals;
    bool                        _deferredValueChangeSignal;
};

#endif
