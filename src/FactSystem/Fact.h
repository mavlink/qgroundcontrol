/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactMetaData.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QDebug>
#include <QAbstractListModel>

class FactValueSliderListModel;

/// @brief A Fact is used to hold a single value within the system.
class Fact : public QObject
{
    Q_OBJECT
    
public:
    Fact(QObject* parent = nullptr);
    Fact(int componentId, QString name, FactMetaData::ValueType_t type, QObject* parent = nullptr);
    Fact(const Fact& other, QObject* parent = nullptr);

    /// Creates a Fact using the name and type from metaData. Also calls QGCCorePlugin::adjustSettingsMetaData allowing
    /// custom builds to override the metadata.
    Fact(const QString& settingsGroup, FactMetaData* metaData, QObject* parent = nullptr);

    const Fact& operator=(const Fact& other);

    Q_PROPERTY(int          componentId             READ componentId                                        CONSTANT)
    Q_PROPERTY(QStringList  bitmaskStrings          READ bitmaskStrings                                     NOTIFY bitmaskStringsChanged)
    Q_PROPERTY(QVariantList bitmaskValues           READ bitmaskValues                                      NOTIFY bitmaskValuesChanged)
    Q_PROPERTY(QStringList  selectedBitmaskStrings  READ selectedBitmaskStrings                             NOTIFY valueChanged)
    Q_PROPERTY(int          decimalPlaces           READ decimalPlaces                                      CONSTANT)
    Q_PROPERTY(QVariant     defaultValue            READ cookedDefaultValue                                 CONSTANT)
    Q_PROPERTY(QString      defaultValueString      READ cookedDefaultValueString                           CONSTANT)
    Q_PROPERTY(bool         defaultValueAvailable   READ defaultValueAvailable                              CONSTANT)
    Q_PROPERTY(int          enumIndex               READ enumIndex              WRITE setEnumIndex          NOTIFY valueChanged)
    Q_PROPERTY(QStringList  enumStrings             READ enumStrings                                        NOTIFY enumsChanged)
    Q_PROPERTY(QString      enumStringValue         READ enumStringValue        WRITE setEnumStringValue    NOTIFY valueChanged)
    Q_PROPERTY(QVariantList enumValues              READ enumValues                                         NOTIFY enumsChanged)
    Q_PROPERTY(QString      category                READ category                                           CONSTANT)
    Q_PROPERTY(QString      group                   READ group                                              CONSTANT)
    Q_PROPERTY(QString      longDescription         READ longDescription                                    CONSTANT)
    Q_PROPERTY(QVariant     max                     READ cookedMax                                          CONSTANT)
    Q_PROPERTY(QString      maxString               READ cookedMaxString                                    CONSTANT)
    Q_PROPERTY(bool         maxIsDefaultForType     READ maxIsDefaultForType                                CONSTANT)
    Q_PROPERTY(QVariant     min                     READ cookedMin                                          CONSTANT)
    Q_PROPERTY(QString      minString               READ cookedMinString                                    CONSTANT)
    Q_PROPERTY(bool         minIsDefaultForType     READ minIsDefaultForType                                CONSTANT)
    Q_PROPERTY(QString      name                    READ name                                               CONSTANT)
    Q_PROPERTY(bool         vehicleRebootRequired   READ vehicleRebootRequired                              CONSTANT)
    Q_PROPERTY(bool         qgcRebootRequired       READ qgcRebootRequired                                  CONSTANT)
    Q_PROPERTY(QString      shortDescription        READ shortDescription                                   CONSTANT)
    Q_PROPERTY(QString      units                   READ cookedUnits                                        CONSTANT)
    Q_PROPERTY(QVariant     value                   READ cookedValue            WRITE setCookedValue        NOTIFY valueChanged)
    Q_PROPERTY(QVariant     rawValue                READ rawValue               WRITE setRawValue           NOTIFY rawValueChanged)
    Q_PROPERTY(bool         valueEqualsDefault      READ valueEqualsDefault                                 NOTIFY valueChanged)
    Q_PROPERTY(QString      valueString             READ cookedValueString                                  NOTIFY valueChanged)
    Q_PROPERTY(QString      enumOrValueString       READ enumOrValueString                                  NOTIFY valueChanged)
    Q_PROPERTY(double       increment               READ cookedIncrement                                    CONSTANT)
    Q_PROPERTY(bool         typeIsString            READ typeIsString                                       CONSTANT)
    Q_PROPERTY(bool         typeIsBool              READ typeIsBool                                         CONSTANT)
    Q_PROPERTY(bool         hasControl              READ hasControl                                         CONSTANT)
    Q_PROPERTY(bool         readOnly                READ readOnly                                           CONSTANT)
    Q_PROPERTY(bool         writeOnly               READ writeOnly                                          CONSTANT)
    Q_PROPERTY(bool         volatileValue           READ volatileValue                                      CONSTANT)

    /// @brief Convert and validate value
    /// @param cookedValue: Value to convert and validate
    /// @param convertOnly true: validate type conversion only, false: validate against meta data as well
    Q_INVOKABLE QString validate(const QString& cookedValue, bool convertOnly);
    /// Convert and clamp value
    Q_INVOKABLE QVariant clamp(const QString& cookedValue);

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
    QStringList     selectedBitmaskStrings  (void) const;
    int             enumIndex               (void);         // This is not const, since an unknown value can modify the enum lists
    QStringList     enumStrings             (void) const;
    QString         enumStringValue         (void);         // This is not const, since an unknown value can modify the enum lists
    QVariantList    enumValues              (void) const;
    QString         category                (void) const;
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
    bool            vehicleRebootRequired   (void) const;
    bool            qgcRebootRequired       (void) const;
    QString         enumOrValueString       (void);         // This is not const, since an unknown value can modify the enum lists
    double          rawIncrement            (void) const;
    double          cookedIncrement         (void) const;
    bool            typeIsString            (void) const { return type() == FactMetaData::valueTypeString; }
    bool            typeIsBool              (void) const { return type() == FactMetaData::valueTypeBool; }
    bool            hasControl              (void) const;
    bool            readOnly                (void) const;
    bool            writeOnly               (void) const;
    bool            volatileValue           (void) const;

    // Internal hack to allow changes to fact which do not signal reboot. Currently used by font point size
    // code in ScreenTools.qml to set initial sizing at first boot.
    Q_INVOKABLE void _setIgnoreQGCRebootRequired(bool ignore);

    Q_INVOKABLE FactValueSliderListModel* valueSliderModel(void);

    /// Returns the values as a string with full 18 digit precision if float/double.
    QString rawValueStringFullPrecision(void) const;

    void setRawValue        (const QVariant& value);
    void setCookedValue     (const QVariant& value);
    void setEnumIndex       (int index);
    void setEnumStringValue (const QString& value);
    int  valueIndex         (const QString& value);

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
    ///     @param metaData FactMetaData for Fact
    ///     @param setDefaultFromMetaData true: set the fact value to the default specified in the meta data
    void setMetaData(FactMetaData* metaData, bool setDefaultFromMetaData = false);
    
    FactMetaData* metaData() { return _metaData; }

    //-- Value coming from Vehicle. This does NOT send a _containerRawValueChanged signal.
    void _containerSetRawValue(const QVariant& value);
    
    /// Generally you should not change the name of a fact. But if you know what you are doing, you can.
    void _setName(const QString& name) { _name = name; }

    /// Generally this is done during parsing. But if you know what you are doing, you can.
    void setEnumInfo(const QStringList& strings, const QVariantList& values);

signals:
    void bitmaskStringsChanged(void);
    void bitmaskValuesChanged(void);
    void enumsChanged(void);
    void sendValueChangedSignalsChanged(bool sendValueChangedSignals);

    /// QObject Property System signal for value property changes
    ///
    /// This signal is only meant for use by the QT property system. It should not be connected to by client code.
    void valueChanged(QVariant value);
    void rawValueChanged(QVariant value);
    
    /// Signalled when the param write ack comes back from the vehicle
    void vehicleUpdated(QVariant value);
    
    /// Signalled when property has been changed by a call to the property write accessor
    ///
    /// This signal is meant for use by Fact container implementations. Used to send changed values to vehicle.
    void _containerRawValueChanged(const QVariant& value);

private slots:
    void _checkForRebootMessaging(void);

private:
    void _init(void);
    
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
    FactValueSliderListModel*   _valueSliderModel;
    bool                        _ignoreQGCRebootRequired;
};
