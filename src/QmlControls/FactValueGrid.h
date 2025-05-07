/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QmlObjectListModel.h"
#include "QGCMAVLink.h"
#include "Vehicle.h"

#include <QtCore/QSettings>
#include <QtQuick/QQuickItem>

class InstrumentValueData;

class FactValueGrid : public QQuickItem
{
    Q_OBJECT

public:
    FactValueGrid(QQuickItem *parent = nullptr);
    ~FactValueGrid();

    enum FontSize {
        DefaultFontSize=0,
        SmallFontSize,
        MediumFontSize,
        LargeFontSize,
    };
    Q_ENUMS(FontSize)

    Q_PROPERTY(QmlObjectListModel*  columns         MEMBER _columns                                     NOTIFY columnsChanged)
    Q_PROPERTY(int                  rowCount        MEMBER _rowCount                                    NOTIFY rowCountChanged)
    Q_PROPERTY(QStringList          iconNames       READ iconNames                                      CONSTANT)
    Q_PROPERTY(FontSize             fontSize        READ fontSize           WRITE setFontSize           NOTIFY fontSizeChanged)
    Q_PROPERTY(QStringList          fontSizeNames   MEMBER _fontSizeNames                               CONSTANT)

    // The following properties should only be set at initial object creation time
    Q_PROPERTY(QString              settingsGroup           MEMBER _settingsGroup           NOTIFY settingsGroupChanged             REQUIRED)
    Q_PROPERTY(Vehicle *            specificVehicleForCard  MEMBER _specificVehicleForCard  NOTIFY specificVehicleForCardChanged    REQUIRED)   ///< null means track active vehicle, set to specific vehicle to track a single vehicle and share settings with other cards

    Q_INVOKABLE void                resetToDefaults (void);
    Q_INVOKABLE QmlObjectListModel* appendColumn    (void);
    Q_INVOKABLE void                deleteLastColumn(void);
    Q_INVOKABLE void                appendRow       (void);
    Q_INVOKABLE void                deleteLastRow   (void);

    QmlObjectListModel*         columns                 (void) const { return _columns; }
    QString                     settingsGroup           (void) const { return _settingsGroup; }
    FontSize                    fontSize                (void) const { return _fontSize; }
    QStringList                 iconNames               (void) const { return _iconNames; }
    QGCMAVLink::VehicleClass_t  vehicleClass            (void) const;
    Vehicle*                    currentVehicle          (void) const { return _specificVehicleForCard ? _specificVehicleForCard : _activeVehicle; }
    Vehicle*                    specificVehicleForCard  (void) const { return _specificVehicleForCard; }

    void setFontSize(FontSize fontSize);

    // Override from QQmlParserStatus
    void componentComplete(void) final;

signals:
    void fontSizeChanged(FontSize fontSize);
    void columnsChanged (QmlObjectListModel* model);
    void rowCountChanged(int rowCount);
    void settingsGroupChanged(QString settingsGroup);
    void specificVehicleForCardChanged(Vehicle* vehicle);

protected:
    Q_DISABLE_COPY(FactValueGrid)

    QString                     _settingsGroup;
    FontSize                    _fontSize               = DefaultFontSize;
    bool                        _preventSaveSettings    = false;
    QmlObjectListModel*         _columns                = nullptr;
    int                         _rowCount               = 0;
    Vehicle*                    _specificVehicleForCard = nullptr;
    Vehicle*                    _activeVehicle          = nullptr;

private slots:
    void _activeVehicleChanged(Vehicle *activeVehicle);
    void _resetFromSettings(void);

private:
    InstrumentValueData*    _createNewInstrumentValueWorker (QObject* parent);
    void                    _saveSettings                   (void);
    void                    _connectSaveSignals             (InstrumentValueData* value);
    QString                 _pascalCase                     (const QString& text);
    void                    _saveValueData                  (QSettings& settings, InstrumentValueData* value);
    void                    _loadValueData                  (QSettings& settings, InstrumentValueData* value);
    QString                 _settingsKey                    (void);
    void                    _initForNewVehicle              (Vehicle* vehicle);
    void                    _deinitVehicle                  (Vehicle* vehicle);

    // These are user facing string for the various enums.
    static       QStringList _iconNames;
    static const QStringList _fontSizeNames;

    static constexpr const char* _columnsKey          = "columns";
    static constexpr const char* _rowsKey             = "rows";
    static constexpr const char* _rowCountKey         = "rowCount";
    static constexpr const char* _fontSizeKey         = "fontSize";
    static constexpr const char* _versionKey          = "version";
    static constexpr const char* _factGroupNameKey    = "factGroupName";
    static constexpr const char* _factNameKey         = "factName";
    static constexpr const char* _textKey             = "text";
    static constexpr const char* _showUnitsKey        = "showUnits";
    static constexpr const char* _iconKey             = "icon";
    static constexpr const char* _rangeTypeKey        = "rangeType";
    static constexpr const char* _rangeValuesKey      = "rangeValues";
    static constexpr const char* _rangeColorsKey      = "rangeColors";
    static constexpr const char* _rangeIconsKey       = "rangeIcons";
    static constexpr const char* _rangeOpacitiesKey   = "rangeOpacities";

    static constexpr const char* _deprecatedGroupKey =  "ValuesWidget";

    static QList<FactValueGrid*> _vehicleCardInstanceList;
};

QML_DECLARE_TYPE(FactValueGrid)

Q_DECLARE_METATYPE(FactValueGrid::FontSize)
