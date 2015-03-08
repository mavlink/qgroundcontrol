/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "FactBinder.h"
#include "UASManager.h"
#include "AutoPilotPluginManager.h"
#include <QDebug>

FactBinder::FactBinder(void) :
    _autopilotPlugin(NULL),
    _fact(NULL)
{
    UASInterface* uas = UASManager::instance()->getActiveUAS();
    Q_ASSERT(uas);
    
    _autopilotPlugin = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(uas);
    Q_ASSERT(_autopilotPlugin);
    Q_ASSERT(_autopilotPlugin->pluginIsReady());
}

QString FactBinder::name(void) const
{
    if (_fact) {
        return _fact->name();
    } else {
        return QString();
    }
}

void FactBinder::setName(const QString& name)
{
    if (_fact) {
        disconnect(_fact, &Fact::valueChanged, this, &FactBinder::valueChanged);
        _fact = NULL;
    }
    
    if (!name.isEmpty()) {
        if (_autopilotPlugin->factExists(name)) {
            _fact = _autopilotPlugin->getFact(name);
            connect(_fact, &Fact::valueChanged, this, &FactBinder::valueChanged);

            emit valueChanged();
            emit nameChanged();
        } else {
            qDebug() << "FAILED BINDING PARAM" << name << ": PARAM DOES NOT EXIST ON SYSTEM!";
        }
    }
}

QVariant FactBinder::value(void) const
{
    if (_fact) {
        return _fact->value();
    } else {
        return QVariant(0);
    }
}

void FactBinder::setValue(const QVariant& value)
{
    if (_fact) {
        _fact->setValue(value);
    } else {
        qDebug() << "FAILED SETTING PARAM VALUE" << value.toString() << ": PARAM DOES NOT EXIST ON SYSTEM!";
    }
}

QString FactBinder::valueString(void) const
{
    if (_fact) {
        return _fact->valueString();
    } else {
        return QString();
    }
}

QString FactBinder::units(void) const
{
    if (_fact) {
        return _fact->units();
    } else {
        return QString();
    }
}
