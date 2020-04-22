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

#include <QQuickItem>

class InstrumentValueData;

class InstrumentValueArea : public QQuickItem
{
    Q_OBJECT

public:
    InstrumentValueArea(QQuickItem *parent = nullptr);
    InstrumentValueArea(const QString& defaultSettingsGroup);

    enum Orientation {
        HorizontalOrientation=0,    // Labels will be to the left of the value
        VerticalOrientation         // Labels will be above the value
    };
    Q_ENUMS(Orientation)

    enum FontSize {
        DefaultFontSize=0,
        SmallFontSize,
        MediumFontSize,
        LargeFontSize,
    };
    Q_ENUMS(FontSize)

    // valuePageDefaultSettingsGroup:
    //  This is the setting group name for default settings which are used when the user has not modified anything from the default setup. These settings will be overwritten
    //  prior to each use by the call to QGCCorePlugin::instrumentValueAreaCreateDefaultSettings.

    // valuePageUserSettingsGroup:
    //  This is the settings group name for user modified settings. Settings will be saved to here whenever the user modified anything. Also at that point in time the
    //  defaults settings group will be removed.

    // The combination of the two valuePage*SettingsGroup values allows each InstrumentValueArea to have it's own persistence space.

    Q_PROPERTY(QString              valuePageDefaultSettingsGroup   MEMBER valuePageDefaultSettingsGroup                    CONSTANT)
    Q_PROPERTY(QString              valuePageUserSettingsGroup      MEMBER _valuePageUserSettingsGroup                      CONSTANT)
    Q_PROPERTY(QString              userSettingsGroup               MEMBER _userSettingsGroup                               NOTIFY userSettingsGroupChanged)
    Q_PROPERTY(QString              defaultSettingsGroup            MEMBER _defaultSettingsGroup                            NOTIFY defaultSettingsGroupChanged)
    Q_PROPERTY(Orientation          orientation                     MEMBER _orientation                                     NOTIFY orientationChanged)
    Q_PROPERTY(QStringList          iconNames                       READ iconNames                                          CONSTANT)
    Q_PROPERTY(FontSize             fontSize                        READ fontSize                       WRITE setFontSize   NOTIFY fontSizeChanged)
    Q_PROPERTY(QStringList          fontSizeNames                   MEMBER _fontSizeNames                                   CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  rowValues                       MEMBER _rowValues                                       NOTIFY rowValuesChanged)

    Q_INVOKABLE InstrumentValueData*    appendColumn        (int rowIndex);
    Q_INVOKABLE void                    deleteLastColumn    (int rowIndex);
    Q_INVOKABLE QmlObjectListModel*     appendRow           (bool addBlanksColumn = true);
    Q_INVOKABLE QmlObjectListModel*     insertRow           (int atIndex, bool addBlanksColumn = true);
    Q_INVOKABLE void                    deleteRow           (int rowIndex);
    Q_INVOKABLE void                    resetToDefaults     (void);

    FontSize            fontSize    (void) const { return _fontSize; }
    QmlObjectListModel* rowValues   (void) { return _rowValues; }
    QStringList         iconNames   (void) const { return _iconNames; }

    void setFontSize(FontSize fontSize);

    // Override from QQmlParserStatus
    void componentComplete(void) final;

    static const QString valuePageDefaultSettingsGroup;

signals:
    void userSettingsGroupChanged   (const QString& userSettingsGroup);
    void defaultSettingsGroupChanged(const QString& defaultSettingsGroup);
    void rowValuesChanged           (QmlObjectListModel* rowValues);
    void orientationChanged         (Orientation orientation);
    void fontSizeChanged            (FontSize fontSize);

private slots:
    void _saveSettings(void);

private:
    void                    _connectSignals                     (void);
    void                    _checkForDeprecatedSettings         (void);
    bool                    _validRowIndex                      (int rowIndex);
    InstrumentValueData*    _createNewInstrumentValueWorker     (QmlObjectListModel* rowModel);
    void                    _loadSettings                       (void);
    QString                 _pascalCase                         (const QString& text);

    Q_DISABLE_COPY(InstrumentValueArea)

    QString                 _defaultSettingsGroup;              // Settings group to read from if the user has not modified from the default settings
    QString                 _userSettingsGroup;                 // Settings group to read from for user modified settings
    Orientation             _orientation =          VerticalOrientation;
    FontSize                _fontSize =             DefaultFontSize;
    QmlObjectListModel*     _rowValues =            nullptr;
    bool                    _preventSaveSettings =  false;

    // These are user facing string for the various enums.
    static       QStringList _iconNames;
    static const QStringList _fontSizeNames;

    static const QString _valuePageUserSettingsGroup;

    static const char* _versionKey;
    static const char* _rowsKey;
    static const char* _columnsKey;
    static const char* _orientationKey;
    static const char* _fontSizeKey;
    static const char* _factGroupNameKey;
    static const char* _factNameKey;
    static const char* _textKey;
    static const char* _showUnitsKey;
    static const char* _iconKey;
    static const char* _rangeTypeKey;
    static const char* _rangeValuesKey;
    static const char* _rangeColorsKey;
    static const char* _rangeIconsKey;
    static const char* _rangeOpacitiesKey;

    static const char* _deprecatedGroupKey;
};

QML_DECLARE_TYPE(InstrumentValueArea)

Q_DECLARE_METATYPE(InstrumentValueArea::FontSize)
Q_DECLARE_METATYPE(InstrumentValueArea::Orientation)
