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

#include "Fact.h"

#include <QtQml>

Fact::Fact(QString name, QObject* parent) :
    QObject(parent),
    _name(name),
    _metaData(NULL)
{
    _value = "";
}

void Fact::setValue(const QVariant& value)
{
    _value = value;
    emit valueChanged(_value);
    emit _containerValueChanged(_value);
}

void Fact::_containerSetValue(const QVariant& value)
{
    _value = value;
    emit valueChanged(_value);
}

QString Fact::name(void) const
{
    return _name;
}

QVariant Fact::value(void) const
{
    return _value;
}

QString Fact::valueString(void) const
{
    return _value.toString();
}

QVariant Fact::defaultValue(void)
{
    return _metaData->defaultValue;
}

FactMetaData::ValueType_t Fact::type(void)
{
    return _metaData->type;
}

QString Fact::shortDescription(void)
{
    return _metaData->shortDescription;
}

QString Fact::longDescription(void)
{
    return _metaData->longDescription;
}

QString Fact::units(void)
{
    return _metaData->units;
}

QVariant Fact::min(void)
{
    return _metaData->min;
}

QVariant Fact::max(void)
{
    return _metaData->max;
}

void Fact::setMetaData(FactMetaData* metaData)
{
    _metaData = metaData;
}
