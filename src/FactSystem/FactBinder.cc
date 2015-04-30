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
#include "QGCApplication.h"

#include <QDebug>

FactBinder::FactBinder(void) :
    _autopilotPlugin(NULL),
    _fact(NULL),
    _componentId(FactSystem::defaultComponentId)
{
    UASInterface* uas = UASManager::instance()->getActiveUAS();
    Q_ASSERT(uas);
    
    _autopilotPlugin = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(uas);
    Q_ASSERT(_autopilotPlugin);
    Q_ASSERT(_autopilotPlugin->pluginReady());
}

QString FactBinder::name(void) const
{
    if (_fact) {
        return _fact->name();
    } else {
        return QString();
    }
}

int FactBinder::componentId(void) const
{
	return _componentId;
}

void FactBinder::setName(const QString& name)
{
    if (_fact) {
        disconnect(_fact, &Fact::valueChanged, this, &FactBinder::valueChanged);
        _fact = NULL;
    }
    
    if (!name.isEmpty()) {
		QString parsedName = name;
		
		// Component id + name combination?
		if (name.contains(":")) {
			QStringList parts = name.split(":");
			if (parts.count() == 2) {
				parsedName = parts[0];
				_componentId = parts[1].toInt();
			}
		}
		
        if (_autopilotPlugin->factExists(FactSystem::ParameterProvider, _componentId, parsedName)) {
            _fact = _autopilotPlugin->getFact(FactSystem::ParameterProvider, _componentId, parsedName);
            connect(_fact, &Fact::valueChanged, this, &FactBinder::valueChanged);

            emit valueChanged();
            emit nameChanged();
			emit metaDataChanged();
		} else {
            QString panicMessage("Required parameter (component id: %1, name: %2),  is missing from vehicle. QGroundControl cannot operate with this firmware revision. QGroundControl will now shut down.");
            qgcApp()->panicShutdown(panicMessage.arg(_componentId).arg(parsedName));
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
        qWarning() << "FAILED SETTING PARAM VALUE" << _fact->name() << ": PARAM DOES NOT EXIST ON SYSTEM!";
        Q_ASSERT(false);
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

QVariant FactBinder::defaultValue(void)
{
    if (_fact) {
        return _fact->defaultValue();
    } else {
		return QVariant(0);
    }
}

FactMetaData::ValueType_t FactBinder::type(void)
{
    if (_fact) {
        return _fact->type();
    } else {
		return FactMetaData::valueTypeUint32;
    }
}

QString FactBinder::shortDescription(void)
{
    if (_fact) {
        return _fact->shortDescription();
    } else {
        return QString();
    }
}

QString FactBinder::longDescription(void)
{
    if (_fact) {
        return _fact->longDescription();
    } else {
        return QString();
    }
}

QVariant FactBinder::min(void)
{
    if (_fact) {
        return _fact->min();
    } else {
		return QVariant(0);
    }
}

QVariant FactBinder::max(void)
{
    if (_fact) {
        return _fact->max();
    } else {
		return QVariant(0);
    }
}

QString FactBinder::group(void)
{
    if (_fact) {
        return _fact->group();
    } else {
        return QString();
    }
}

bool FactBinder::defaultValueAvailable(void)
{
    if (_fact) {
        return _fact->defaultValueAvailable();
    } else {
        return false;
    }
}

bool FactBinder::valueEqualsDefault(void)
{
    if (_fact) {
        return _fact->valueEqualsDefault();
    } else {
        return false;
    }
}
