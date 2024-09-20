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

#include <QtCore/QSettings>
#include <QtQuick/QQuickItem>

class InstrumentValueData;

class FactValueGrid : public QQuickItem
{
    Q_OBJECT

public:
    FactValueGrid(QQuickItem *parent = nullptr);
    FactValueGrid(const QString& defaultSettingsGroup);

    enum FontSize {
        DefaultFontSize=0,
        SmallFontSize,
        MediumFontSize,
        LargeFontSize,
    };
    Q_ENUMS(FontSize)

    // defaultSettingsGroup:
    //  This is the setting group name for default settings which are used when the user has not modified anything from the default setup. These settings will be overwritten
    //  prior to each use by the call to QGCCorePlugin::FactValueGridCreateDefaultSettings.

    // userSettingsGroup:
    //  This is the settings group name for user modified settings. Settings will be saved to here whenever the user modified anything. Also at that point in time the
    //  defaults settings group will be removed.

    // The combination of the two valuePage*SettingsGroup values allows each FactValueGrid to have it's own persistence space.

    Q_PROPERTY(QmlObjectListModel*  columns                         MEMBER _columns                                         NOTIFY columnsChanged)
    Q_PROPERTY(int                  rowCount                        MEMBER _rowCount                                        NOTIFY rowCountChanged)
    Q_PROPERTY(QString              userSettingsGroup               MEMBER _userSettingsGroup                               NOTIFY userSettingsGroupChanged)
    Q_PROPERTY(QString              defaultSettingsGroup            MEMBER _defaultSettingsGroup                            NOTIFY defaultSettingsGroupChanged)
    Q_PROPERTY(QStringList          iconNames                       READ iconNames                                          CONSTANT)
    Q_PROPERTY(FontSize             fontSize                        READ fontSize                       WRITE setFontSize   NOTIFY fontSizeChanged)
    Q_PROPERTY(QStringList          fontSizeNames                   MEMBER _fontSizeNames                                   CONSTANT)

    Q_INVOKABLE void                resetToDefaults (void);
    Q_INVOKABLE QmlObjectListModel* appendColumn    (void);
    Q_INVOKABLE void                deleteLastColumn(void);
    Q_INVOKABLE void                appendRow       (void);
    Q_INVOKABLE void                deleteLastRow   (void);

    QmlObjectListModel*         columns     (void) const { return _columns; }
    FontSize                    fontSize    (void) const { return _fontSize; }
    QStringList                 iconNames   (void) const { return _iconNames; }
    QGCMAVLink::VehicleClass_t  vehicleClass(void) const { return _vehicleClass; }

    void setFontSize(FontSize fontSize);

    // This is only exposed for usage of FactValueGrid to be able to just read the settings and display no ui. For this case
    // create a FactValueGrid object with a null parent. Set the userSettingsGroup/defaultSettingsGroup appropriately and then
    // call _loadSettings. Then after that you can read the settings from the object. You should not change any of the values.
    // Destroy the FactValueGrid object when done.
    void _loadSettings(void);

    // Override from QQmlParserStatus
    void componentComplete(void) final;

signals:
    void userSettingsGroupChanged   (const QString& userSettingsGroup);
    void defaultSettingsGroupChanged(const QString& defaultSettingsGroup);
    void fontSizeChanged            (FontSize fontSize);
    void columnsChanged             (QmlObjectListModel* model);
    void rowCountChanged            (int rowCount);

protected:
    Q_DISABLE_COPY(FactValueGrid)

    QGCMAVLink::VehicleClass_t  _vehicleClass           = QGCMAVLink::VehicleClassGeneric;
    QString                     _defaultSettingsGroup;                                      // Settings group to read from if the user has not modified from the default settings
    QString                     _userSettingsGroup;                                         // Settings group to read from for user modified settings
    FontSize                    _fontSize               = DefaultFontSize;
    bool                        _preventSaveSettings    = false;
    QmlObjectListModel*         _columns                = nullptr;
    int                         _rowCount               = 0;

private slots:
    void _offlineVehicleTypeChanged(void);

private:
    InstrumentValueData*    _createNewInstrumentValueWorker (QObject* parent);
    void                    _saveSettings                   (void);
    void                    _init                           (void);
    void                    _connectSaveSignals             (InstrumentValueData* value);
    QString                 _pascalCase                     (const QString& text);
    void                    _saveValueData                  (QSettings& settings, InstrumentValueData* value);
    void                    _loadValueData                  (QSettings& settings, InstrumentValueData* value);

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
};

QML_DECLARE_TYPE(FactValueGrid)

Q_DECLARE_METATYPE(FactValueGrid::FontSize)
