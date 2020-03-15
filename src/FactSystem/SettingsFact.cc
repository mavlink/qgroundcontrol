/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "SettingsFact.h"
#include "QGCCorePlugin.h"
#include "QGCApplication.h"

#include <QSettings>

SettingsFact::SettingsFact(QObject* parent)
    : Fact(parent)
{    
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
}

SettingsFact::SettingsFact(QString settingsGroup, FactMetaData* metaData, QObject* parent)
    : Fact          (0, metaData->name(), metaData->type(), parent)
    , _settingsGroup(settingsGroup)
    , _visible      (true)
{
    QSettings settings;

    if (!_settingsGroup.isEmpty()) {
        settings.beginGroup(_settingsGroup);
    }

    // Allow core plugin a chance to override the default value
    _visible = qgcApp()->toolbox()->corePlugin()->adjustSettingMetaData(settingsGroup, *metaData);
    setMetaData(metaData);

    if (metaData->defaultValueAvailable()) {
        QVariant rawDefaultValue = metaData->rawDefaultValue();
        if (qgcApp()->runningUnitTests()) {
            // Don't use saved settings
            _rawValue = rawDefaultValue;
        } else {
            if (_visible) {
                QVariant typedValue;
                QString errorString;
                metaData->convertAndValidateRaw(settings.value(_name, rawDefaultValue), true /* conertOnly */, typedValue, errorString);
                _rawValue = typedValue;
            } else {
                // Setting is not visible, force to default value always
                settings.setValue(_name, rawDefaultValue);
                _rawValue = rawDefaultValue;
            }
        }
    }

    connect(this, &Fact::rawValueChanged, this, &SettingsFact::_rawValueChanged);
}

SettingsFact::SettingsFact(const SettingsFact& other, QObject* parent)
    : Fact(other, parent)
{
    *this = other;
}

const SettingsFact& SettingsFact::operator=(const SettingsFact& other)
{
    Fact::operator=(other);
    
    _settingsGroup = other._settingsGroup;

    return *this;
}

void SettingsFact::_rawValueChanged(QVariant value)
{
    QSettings settings;

    if (!_settingsGroup.isEmpty()) {
        settings.beginGroup(_settingsGroup);
    }

    settings.setValue(_name, value);
}
