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

class InstrumentValue : public QObject
{
    Q_OBJECT

public:
    enum FontSize {
        DefaultFontSize=0,
        SmallFontSize,
        MediumFontSize,
        LargeFontSize,
    };
    Q_ENUMS(FontSize)

    enum LabelPosition {
        LabelAbove = 0,
        LabelLeft,
    };
    Q_ENUMS(LabelPosition)

    enum RangeType {
        NoRangeInfo = 0,
        ColorRange,
        OpacityRange,
        IconSelectRange,
    };
    Q_ENUMS(RangeType)

    InstrumentValue(Vehicle* activeVehicle, FontSize fontSize, QmlObjectListModel* rowModel);

    Q_PROPERTY(QStringList      factGroupNames      MEMBER  _factGroupNames                             NOTIFY factGroupNamesChanged)
    Q_PROPERTY(QStringList      factValueNames      MEMBER  _factValueNames                             NOTIFY factValueNamesChanged)
    Q_PROPERTY(QString          factGroupName       MEMBER  _factGroupName                              NOTIFY factGroupNameChanged)
    Q_PROPERTY(QString          factName            MEMBER  _factName                                   NOTIFY factNameChanged)
    Q_PROPERTY(Fact*            fact                READ    fact                                        NOTIFY factChanged)
    Q_PROPERTY(QString          text                READ    text                WRITE setText           NOTIFY textChanged)
    Q_PROPERTY(QString          icon                READ    icon                WRITE setIcon           NOTIFY iconChanged)             ///< If !isEmpty icon will be show instead of label
    Q_PROPERTY(LabelPosition    labelPosition       READ    labelPosition       WRITE setLabelPosition  NOTIFY labelPositionChanged)
    Q_PROPERTY(QStringList      labelPositionNames  MEMBER _labelPositionNames                          CONSTANT)
    Q_PROPERTY(QStringList      iconNames           MEMBER _iconNames                                   CONSTANT)
    Q_PROPERTY(FontSize         fontSize            READ    fontSize            WRITE setFontSize       NOTIFY fontSizeChanged)
    Q_PROPERTY(QStringList      fontSizeNames       MEMBER _fontSizeNames                               CONSTANT)
    Q_PROPERTY(bool             showUnits           READ    showUnits           WRITE setShowUnits      NOTIFY showUnitsChanged)
    Q_PROPERTY(QStringList      rangeTypeNames      MEMBER _rangeTypeNames                              CONSTANT)
    Q_PROPERTY(RangeType        rangeType           READ    rangeType           WRITE setRangeType      NOTIFY rangeTypeChanged)
    Q_PROPERTY(QVariantList     rangeValues         READ    rangeValues         WRITE setRangeValues    NOTIFY rangeValuesChanged)
    Q_PROPERTY(QVariantList     rangeColors         READ    rangeColors         WRITE setRangeColors    NOTIFY rangeColorsChanged)
    Q_PROPERTY(QVariantList     rangeIcons          READ    rangeIcons          WRITE setRangeIcons     NOTIFY rangeIconsChanged)
    Q_PROPERTY(QVariantList     rangeOpacities      READ    rangeOpacities      WRITE setRangeOpacities NOTIFY rangeOpacitiesChanged)
    Q_PROPERTY(QColor           currentColor        MEMBER _currentColor                                NOTIFY currentColorChanged)
    Q_PROPERTY(double           currentOpacity      MEMBER _currentOpacity                              NOTIFY currentOpacityChanged)
    Q_PROPERTY(QString          currentIcon         MEMBER _currentIcon                                 NOTIFY currentIconChanged)

    Q_INVOKABLE void    setFact         (const QString& factGroupName, const QString& factName);
    Q_INVOKABLE void    clearFact       (void);
    Q_INVOKABLE bool    isValidColor    (const QColor& color)   { return color.isValid(); }
    Q_INVOKABLE QColor  invalidColor    (void)                  { return QColor(); }
    Q_INVOKABLE void    addRangeValue   (void);
    Q_INVOKABLE void    removeRangeValue(int index);

    Fact*           fact                    (void) { return _fact; }
    FontSize        fontSize                (void) const { return _fontSize; }
    QString         text                    (void) const { return _text; }
    bool            showUnits               (void) const { return _showUnits; }
    QString         icon                    (void) const { return _icon; }
    LabelPosition   labelPosition           (void) const { return _labelPosition; }
    RangeType       rangeType               (void) const { return _rangeType; }
    QVariantList    rangeValues             (void) const { return _rangeValues; }
    QVariantList    rangeColors             (void) const { return _rangeColors; }
    QVariantList    rangeIcons              (void) const { return _rangeIcons; }
    QVariantList    rangeOpacities          (void) const { return _rangeOpacities; }
    void            setFontSize             (FontSize fontSize);
    void            setText                 (const QString& text);
    void            setShowUnits            (bool showUnits);
    void            setIcon                 (const QString& icon);
    void            setLabelPosition        (LabelPosition labelPosition);
    void            setRangeType            (RangeType rangeType);
    void            setRangeValues          (const QVariantList& rangeValues);
    void            setRangeColors          (const QVariantList& rangeColors);
    void            setRangeIcons           (const QVariantList& rangeIcons);
    void            setRangeOpacities       (const QVariantList& rangeOpacities);
    void            activeVehicleChanged    (Vehicle* activeVehicle);
    void            saveToSettings          (QSettings& settings) const;
    void            readFromSettings        (const QSettings& settings);

signals:
    void factChanged            (Fact* fact);
    void factNameChanged        (const QString& factName);
    void factGroupNameChanged   (const QString& factGroup);
    void textChanged            (QString text);
    void fontSizeChanged        (FontSize fontSize);
    void showUnitsChanged       (bool showUnits);
    void iconChanged            (const QString& icon);
    void labelPositionChanged   (LabelPosition labelPosition);
    void factGroupNamesChanged  (const QStringList& factGroupNames);
    void factValueNamesChanged  (const QStringList& factValueNames);
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

private:
    void _setFontSize           (FontSize fontSize);
    int  _currentRangeIndex     (const QVariant& value);
    void _updateColor           (void);
    void _updateIcon            (void);
    void _updateOpacity         (void);

    Vehicle*            _activeVehicle =    nullptr;
    QmlObjectListModel* _rowModel =         nullptr;
    Fact*               _fact =             nullptr;
    QString             _factName;
    QString             _factGroupName;
    QString             _text;
    bool                _showUnits =        true;
    FontSize            _fontSize =         DefaultFontSize;
    QString             _icon;
    LabelPosition       _labelPosition =    LabelLeft;
    QStringList         _factGroupNames;
    QStringList         _factValueNames;
    QColor              _currentColor;
    double              _currentOpacity =   1.0;
    QString             _currentIcon;

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
    static const QStringList _labelPositionNames;
    static       QStringList _iconNames;
    static const QStringList _fontSizeNames;

    static const char*  _versionKey;
    static const char*  _factGroupNameKey;
    static const char*  _factNameKey;
    static const char*  _textKey;
    static const char*  _fontSizeKey;
    static const char*  _showUnitsKey;
    static const char*  _iconKey;
    static const char*  _labelPositionKey;
    static const char*  _rangeTypeKey;
    static const char*  _rangeValuesKey;
    static const char*  _rangeColorsKey;
    static const char*  _rangeIconsKey;
    static const char*  _rangeOpacitiesKey;
    static const char*  _vehicleFactGroupName;
};

Q_DECLARE_METATYPE(InstrumentValue::FontSize)
Q_DECLARE_METATYPE(InstrumentValue::LabelPosition)
Q_DECLARE_METATYPE(InstrumentValue::RangeType)
