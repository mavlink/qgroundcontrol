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

    InstrumentValue(Vehicle* activeVehicle, int fontSize, QmlObjectListModel* rowModel);

    Q_PROPERTY(QString  factGroupName   MEMBER  _factGroupName                      NOTIFY factGroupNameChanged)
    Q_PROPERTY(Fact*    fact            READ    fact                                NOTIFY factChanged)
    Q_PROPERTY(QString  label           READ    label           WRITE setLabel      NOTIFY labelChanged)
    Q_PROPERTY(int      fontSize        READ    fontSize        WRITE setFontSize   NOTIFY fontSizeChanged)
    Q_PROPERTY(bool     showUnits       READ    showUnits       WRITE setShowUnits  NOTIFY showUnitsChanged)

    Q_INVOKABLE void setFact(QString factGroupName, QString factName, QString label);
    Q_INVOKABLE void clearFact(void);

    Fact*   fact                    (void) { return _fact; }
    int     fontSize                (void) const { return _fontSize; }
    QString label                   (void) const { return _label; }
    bool    showUnits               (void) const { return _showUnits; }
    void    setFontSize             (int fontSize);
    void    setLabel                (const QString& label);
    void    setShowUnits            (bool showUnits);
    void    activeVehicleChanged    (Vehicle* activeVehicle);
    void    saveToSettings          (QSettings& settings) const;
    void    readFromSettings        (const QSettings& settings);

signals:
    void factChanged            (Fact* fact);
    void factGroupNameChanged   (QString factGroup);
    void labelChanged           (QString label);
    void fontSizeChanged        (int fontSize);
    void showUnitsChanged       (bool showUnits);

private slots:

private:
    void _setFontSize(int fontSize);

    Vehicle*            _activeVehicle =    nullptr;
    QmlObjectListModel* _rowModel =         nullptr;
    Fact*               _fact =             nullptr;
    QString             _factGroupName;
    QString             _label;
    bool                _showUnits =        true;
    int                 _fontSize =         DefaultFontSize;

    static const char*  _factGroupNameKey;
    static const char*  _factNameKey;
    static const char*  _labelKey;
    static const char*  _fontSizeKey;
    static const char*  _showUnitsKey;
};

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

signals:
    void valuesModelChanged(QmlObjectListModel* valuesModel);

private slots:
    void        _activeVehicleChanged(Vehicle* activeVehicle);
    Vehicle*    _currentActiveVehicle(void);
    void        _saveSettings  (void);

private:
    bool                _validRowIndex                      (int rowIndex);
    InstrumentValue*    _createNewInstrumentValueWorker     (Vehicle* activeVehicle, int fontSize, QmlObjectListModel* rowModel);
    void                _loadSettings                       (void);

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
