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
#include "InstrumentValue.h"

#include <QObject>

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
