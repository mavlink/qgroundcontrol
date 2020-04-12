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

class ValuesWidgetController;

class InstrumentValue : public QObject
{
    Q_OBJECT

public:
    enum FontSize {
        DefaultFontSize=0,
        SmallFontSize,
        MediumFontSize,
        LargeFontSize
    };
    Q_ENUMS(FontSize)

    enum IconPosition {
        IconAbove = 0,
        IconLeft
    };
    Q_ENUMS(IconPosition)

    InstrumentValue(Vehicle* activeVehicle, FontSize fontSize, QmlObjectListModel* rowModel);

    Q_PROPERTY(QStringList  factGroupNames      MEMBER  _factGroupNames                             NOTIFY factGroupNamesChanged)
    Q_PROPERTY(QStringList  factValueNames      MEMBER  _factValueNames                             NOTIFY factValueNamesChanged)
    Q_PROPERTY(QString      factGroupName       MEMBER  _factGroupName                              NOTIFY factGroupNameChanged)
    Q_PROPERTY(QString      factName            MEMBER  _factName                                   NOTIFY factNameChanged)
    Q_PROPERTY(Fact*        fact                READ    fact                                        NOTIFY factChanged)
    Q_PROPERTY(QString      label               READ    label               WRITE setLabel          NOTIFY labelChanged)
    Q_PROPERTY(QString      icon                READ    icon                WRITE setIcon           NOTIFY iconChanged)             ///< If !isEmpty icon will be show instead of label
    Q_PROPERTY(IconPosition iconPosition        READ    iconPosition        WRITE setIconPosition   NOTIFY iconPositionChanged)
    Q_PROPERTY(QStringList  iconPositionNames   MEMBER _iconPositionNames                           CONSTANT)
    Q_PROPERTY(QStringList  iconNames           MEMBER _iconNames                                   CONSTANT)
    Q_PROPERTY(FontSize     fontSize            READ    fontSize            WRITE setFontSize       NOTIFY fontSizeChanged)
    Q_PROPERTY(QStringList  fontSizeNames       MEMBER _fontSizeNames                               CONSTANT)
    Q_PROPERTY(bool         showUnits           READ    showUnits           WRITE setShowUnits      NOTIFY showUnitsChanged)

    Q_INVOKABLE void    setFact(const QString& factGroupName, const QString& factName);
    Q_INVOKABLE void    clearFact(void);

    Fact*           fact                    (void) { return _fact; }
    FontSize        fontSize                (void) const { return _fontSize; }
    QString         label                   (void) const { return _label; }
    bool            showUnits               (void) const { return _showUnits; }
    QString         icon                    (void) const { return _icon; }
    IconPosition    iconPosition            (void) const { return _iconPosition; }
    void            setFontSize             (FontSize fontSize);
    void            setLabel                (const QString& label);
    void            setShowUnits            (bool showUnits);
    void            setIcon                 (const QString& icon);
    void            setIconPosition         (IconPosition iconPosition);
    void            activeVehicleChanged    (Vehicle* activeVehicle);
    void            saveToSettings          (QSettings& settings) const;
    void            readFromSettings        (const QSettings& settings);

signals:
    void factChanged            (Fact* fact);
    void factNameChanged        (const QString& factName);
    void factGroupNameChanged   (const QString& factGroup);
    void labelChanged           (QString label);
    void fontSizeChanged        (FontSize fontSize);
    void showUnitsChanged       (bool showUnits);
    void iconChanged            (const QString& icon);
    void iconPositionChanged    (IconPosition iconPosition);
    void factGroupNamesChanged  (const QStringList& factGroupNames);
    void factValueNamesChanged  (const QStringList& factValueNames);

private:
    void _setFontSize           (FontSize fontSize);

    Vehicle*            _activeVehicle =    nullptr;
    QmlObjectListModel* _rowModel =         nullptr;
    Fact*               _fact =             nullptr;
    QString             _factName;
    QString             _factGroupName;
    QString             _label;
    bool                _showUnits =        true;
    FontSize            _fontSize =         DefaultFontSize;
    QString             _icon;
    IconPosition        _iconPosition =     IconLeft;
    QStringList         _factGroupNames;
    QStringList         _factValueNames;

    static const QStringList _iconPositionNames;
    static       QStringList _iconNames;
    static const QStringList _fontSizeNames;

    static const char*  _factGroupNameKey;
    static const char*  _factNameKey;
    static const char*  _labelKey;
    static const char*  _fontSizeKey;
    static const char*  _showUnitsKey;
    static const char*  _iconKey;
    static const char*  _iconPositionKey;
    static const char*  _vehicleFactGroupName;
};

Q_DECLARE_METATYPE(InstrumentValue::FontSize)
Q_DECLARE_METATYPE(InstrumentValue::IconPosition)

class ValuesWidgetController : public QObject
{
    Q_OBJECT
    
public:
    ValuesWidgetController(bool forDefaultSettingsCreation = false);

    Q_PROPERTY(QmlObjectListModel* valuesModel READ valuesModel NOTIFY valuesModelChanged)

    Q_INVOKABLE InstrumentValue*    appendColumn        (int rowIndex);
    Q_INVOKABLE void                deleteLastColumn    (int rowIndex);
    Q_INVOKABLE QmlObjectListModel* appendRow           (bool addBlanksColumn = true);
    Q_INVOKABLE QmlObjectListModel* insertRow           (int atIndex, bool addBlanksColumn = true);
    Q_INVOKABLE void                deleteRow           (int rowIndex);
    Q_INVOKABLE int                 fontSizeForRow      (int rowIndex);
    Q_INVOKABLE void                setFontSizeForRow   (int rowIndex, int fontSize);
    Q_INVOKABLE void                resetToDefaults     (void);

    QmlObjectListModel* valuesModel(void) { return _valuesModel; }

    /// Turn on/off saving changes to QSettings
    void setPreventSaveSettings(bool preventSaveSettings);

    /// Allows the ownership of the _valuesModel to be re-parented to a different controller
    void setValuesModelParentController(ValuesWidgetController* newParentController);

signals:
    void valuesModelChanged(QmlObjectListModel* valuesModel);

private slots:
    void        _activeVehicleChanged(Vehicle* activeVehicle);
    Vehicle*    _currentActiveVehicle(void);
    void        _saveSettings  (void);

private:
    bool                _validRowIndex                      (int rowIndex);
    InstrumentValue*    _createNewInstrumentValueWorker     (Vehicle* activeVehicle, InstrumentValue::FontSize fontSize, QmlObjectListModel* rowModel);
    void                _loadSettings                       (void);
    void                _connectSignalsToController         (InstrumentValue* value, ValuesWidgetController* controller);
    QString             _pascalCase                         (const QString& text);

    MultiVehicleManager*    _multiVehicleMgr =      nullptr;
    QmlObjectListModel*     _valuesModel =          nullptr;
    QVariantList            _rgFontSizeByRow;
    bool                    _preventSaveSettings =  false;

    static const char* _groupKey;
    static const char* _rowsKey;
    static const char* _columnsKey;

    static const char* _deprecatedGroupKey;
    static const char* _deprecatedLargeValuesKey;
    static const char* _deprecatedSmallValuesKey;

};
