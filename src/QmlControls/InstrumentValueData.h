/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "FactSystem.h"
#include "QmlObjectListModel.h"
#include "QGCApplication.h"

#include <QObject>

class FactValueGrid;

class InstrumentValueData : public QObject
{
    Q_OBJECT

public:
    enum RangeType {
        NoRangeInfo = 0,
        ColorRange,
        OpacityRange,
        IconSelectRange,
    };
    Q_ENUMS(RangeType)

    explicit InstrumentValueData(FactValueGrid* factValueGrid, QObject* parent);

    Q_PROPERTY(FactValueGrid*       factValueGrid       MEMBER _factValueGrid                               CONSTANT)
    Q_PROPERTY(QStringList          factGroupNames      READ    factGroupNames                              NOTIFY factGroupNamesChanged)
    Q_PROPERTY(QStringList          factValueNames      READ    factValueNames                              NOTIFY factValueNamesChanged)
    Q_PROPERTY(QString              factGroupName       READ    factGroupName                               NOTIFY factGroupNameChanged)
    Q_PROPERTY(QString              factName            READ    factName                                    NOTIFY factNameChanged)
    Q_PROPERTY(Fact*                fact                READ    fact                                        NOTIFY factChanged)
    Q_PROPERTY(QString              text                READ    text                WRITE setText           NOTIFY textChanged)
    Q_PROPERTY(QString              icon                READ    icon                WRITE setIcon           NOTIFY iconChanged)             ///< If !isEmpty icon will be show instead of label
    Q_PROPERTY(bool                 showUnits           READ    showUnits           WRITE setShowUnits      NOTIFY showUnitsChanged)
    Q_PROPERTY(QStringList          rangeTypeNames      MEMBER _rangeTypeNames                              CONSTANT)
    Q_PROPERTY(RangeType            rangeType           READ    rangeType           WRITE setRangeType      NOTIFY rangeTypeChanged)
    Q_PROPERTY(QVariantList         rangeValues         READ    rangeValues         WRITE setRangeValues    NOTIFY rangeValuesChanged)
    Q_PROPERTY(QVariantList         rangeColors         READ    rangeColors         WRITE setRangeColors    NOTIFY rangeColorsChanged)
    Q_PROPERTY(QVariantList         rangeIcons          READ    rangeIcons          WRITE setRangeIcons     NOTIFY rangeIconsChanged)
    Q_PROPERTY(QVariantList         rangeOpacities      READ    rangeOpacities      WRITE setRangeOpacities NOTIFY rangeOpacitiesChanged)
    Q_PROPERTY(QColor               currentColor        MEMBER _currentColor                                NOTIFY currentColorChanged)
    Q_PROPERTY(double               currentOpacity      MEMBER _currentOpacity                              NOTIFY currentOpacityChanged)
    Q_PROPERTY(QString              currentIcon         MEMBER _currentIcon                                 NOTIFY currentIconChanged)

    Q_INVOKABLE void    setFact         (const QString& factGroupName, const QString& factName);
    Q_INVOKABLE void    clearFact       (void);
    Q_INVOKABLE bool    isValidColor    (const QColor& color)   { return color.isValid(); }
    Q_INVOKABLE QColor  invalidColor    (void)                  { return QColor(); }
    Q_INVOKABLE void    addRangeValue   (void);
    Q_INVOKABLE void    removeRangeValue(int index);

    QStringList     factGroupNames          (void) const;
    QStringList     factValueNames          (void) const;
    QString         factGroupName           (void) const { return _factGroupName; }
    QString         factName                (void) const { return _factName; }
    Fact*           fact                    (void) { return _fact; }
    QString         text                    (void) const { return _text; }
    bool            showUnits               (void) const { return _showUnits; }
    QString         icon                    (void) const { return _icon; }
    RangeType       rangeType               (void) const { return _rangeType; }
    QVariantList    rangeValues             (void) const { return _rangeValues; }
    QVariantList    rangeColors             (void) const { return _rangeColors; }
    QVariantList    rangeIcons              (void) const { return _rangeIcons; }
    QVariantList    rangeOpacities          (void) const { return _rangeOpacities; }
    void            setText                 (const QString& text);
    void            setShowUnits            (bool showUnits);
    void            setIcon                 (const QString& icon);
    void            setRangeType            (RangeType rangeType);
    void            setRangeValues          (const QVariantList& rangeValues);
    void            setRangeColors          (const QVariantList& rangeColors);
    void            setRangeIcons           (const QVariantList& rangeIcons);
    void            setRangeOpacities       (const QVariantList& rangeOpacities);


    static const char*  vehicleFactGroupName;

signals:
    void factChanged            (Fact* fact);
    void factNameChanged        (const QString& factName);
    void factGroupNameChanged   (const QString& factGroup);
    void textChanged            (QString text);
    void showUnitsChanged       (bool showUnits);
    void iconChanged            (const QString& icon);
    void factGroupNamesChanged  (void);
    void factValueNamesChanged  (void);
    void rangeTypeChanged       (RangeType rangeType);
    void rangeValuesChanged     (const QVariantList& rangeValues);
    void rangeColorsChanged     (const QVariantList& rangeColors);
    void rangeIconsChanged      (const QVariantList& rangeIcons);
    void rangeOpacitiesChanged  (const QVariantList& rangeOpacities);
    void currentColorChanged    (const QColor& currentColor);
    void currentOpacityChanged  (double currentOpacity);
    void currentIconChanged     (const QString& currentIcon);

private slots:
    void _resetRangeInfo        (void);
    void _updateRanges          (void);
    void _activeVehicleChanged  (Vehicle* activeVehicle);
    void _lookForMissingFact    (void);

private:
    int  _currentRangeIndex     (const QVariant& value);
    void _updateColor           (void);
    void _updateIcon            (void);
    void _updateOpacity         (void);
    void _setFactWorker         (void);

    FactValueGrid*          _factValueGrid =        nullptr;
    Vehicle*                _activeVehicle =        nullptr;
    QmlObjectListModel*     _rowModel =             nullptr;
    Fact*                   _fact =                 nullptr;
    QString                 _factName;
    QString                 _factGroupName;
    QString                 _text;
    bool                    _showUnits =            true;
    QString                 _icon;
    QColor                  _currentColor;
    double                  _currentOpacity =       1.0;
    QString                 _currentIcon;

    // Ranges allow you to specifiy semantics to apply when a value is within a certain range.
    // The limits for each section of the range are specified in _rangeValues. With the first
    // element indicating a range from that value to -infinity and the last element indicating
    // a range from the value to +infinity.
    //
    // The semantics to apply are defined by the _rangeType value. With the semantic lists having
    // a specific value for each section of the range. There should be _rangeValues.count() + 2
    // semantic values in the apppropriate list.
    RangeType           _rangeType =        NoRangeInfo;
    QVariantList        _rangeValues;                       ///< double values which indicate range setpoints
    QVariantList        _rangeColors;                       ///< QColor
    QVariantList        _rangeIcons;                        ///< QString resource name
    QVariantList        _rangeOpacities;                    /// double opacity value

    // These are user facing string for the various enums.
    static const QStringList _rangeTypeNames;

};

QML_DECLARE_TYPE(InstrumentValueData)

Q_DECLARE_METATYPE(InstrumentValueData::RangeType)
