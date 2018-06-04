/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef SettingsGroup_H
#define SettingsGroup_H

#include "QGCLoggingCategory.h"
#include "Joystick.h"
#include "MultiVehicleManager.h"
#include "QGCToolbox.h"

#include <QVariantList>

#define DEFINE_SETTINGGROUP(CLASS) \
    static const char* CLASS ## Settings ## GroupName;

#define DECLARE_SETTINGGROUP(CLASS) \
    const char* CLASS ## Settings::CLASS ## Settings ## GroupName = #CLASS; \
    CLASS ## Settings::CLASS ## Settings(QObject* parent) \
        : SettingsGroup(CLASS ## Settings ## GroupName, QString() /* root settings group */, parent)

#define DECLARE_SETTINGSFACT(CLASS, NAME) \
    const char* CLASS::NAME ## Name = #NAME; \
    Fact* CLASS::NAME(void) \
    { \
        if (!_ ## NAME ## Fact) { \
            _ ## NAME ## Fact = _createSettingsFact(NAME ## Name); \
        } \
        return _ ## NAME ## Fact; \
    }

#define DEFINE_SETTINGFACT(NAME) \
    public: \
    Q_PROPERTY(Fact* NAME READ NAME CONSTANT) \
    Fact* NAME(); \
    static const char* NAME ## Name; \
    private: \
    SettingsFact* _ ## NAME ## Fact;

#define INIT_SETTINGFACT(NAME) _ ## NAME ## Fact = NULL

/// Provides access to group of settings. The group is named and has a visible property associated with which can control whether the group
/// is shows in the ui.
class SettingsGroup : public QObject
{
    Q_OBJECT
    
public:
    /// @param name Name for this Settings group
    /// @param settingsGroup Group to place settings in for QSettings::setGroup
    SettingsGroup(const QString& name, const QString& settingsGroup, QObject* parent = NULL);

    Q_PROPERTY(bool visible MEMBER _visible CONSTANT)

protected:
    SettingsFact* _createSettingsFact(const QString& factName);

    QString _name;              ///< Name for group. Used to generate name for loaded json meta data file.
    QString _settingsGroup;     ///< QSettings group which contains these settings. empty for settings in root
    bool    _visible;

    QMap<QString, FactMetaData*> _nameToMetaDataMap;
};

#endif
