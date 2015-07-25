/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/*!
    @file
       @brief Link specific configuration base class
       @author Gus Grubba <mavlink@grubba.com>
*/

#include "LinkConfiguration.h"
#ifndef __ios__
#include "SerialLink.h"
#endif
#include "UDPLink.h"
#include "TCPLink.h"
#include "LogReplayLink.h"

#ifdef QT_DEBUG
#include "MockLink.h"
#endif

#define LINK_SETTING_ROOT "LinkConfigurations"

LinkConfiguration::LinkConfiguration(const QString& name)
    : _preferred(false)
    , _dynamic(false)
{
    _link = NULL;
    _name = name;
    Q_ASSERT(!_name.isEmpty());
}

LinkConfiguration::LinkConfiguration(LinkConfiguration* copy)
{
    _link       = copy->getLink();
    _name       = copy->name();
    _preferred  = copy->isPreferred();
    _dynamic    = copy->isDynamic();
    Q_ASSERT(!_name.isEmpty());
}

void LinkConfiguration::copyFrom(LinkConfiguration* source)
{
    Q_ASSERT(source != NULL);
    _link       = source->getLink();
    _name       = source->name();
    _preferred  = source->isPreferred();
    _dynamic    = source->isDynamic();
}

/*!
  Where the settings are saved
  @return The root path of the setting.
*/
const QString LinkConfiguration::settingsRoot()
{
    return QString(LINK_SETTING_ROOT);
}

/*!
  Configuration Factory
  @return A new instance of the given type
*/
LinkConfiguration* LinkConfiguration::createSettings(int type, const QString& name)
{
    LinkConfiguration* config = NULL;
    switch(type) {
#ifndef __ios__
        case LinkConfiguration::TypeSerial:
            config = new SerialConfiguration(name);
            break;
#endif
        case LinkConfiguration::TypeUdp:
            config = new UDPConfiguration(name);
            break;
        case LinkConfiguration::TypeTcp:
            config = new TCPConfiguration(name);
            break;
        case LinkConfiguration::TypeLogReplay:
            config = new LogReplayLinkConfiguration(name);
            break;
#ifdef QT_DEBUG
        case LinkConfiguration::TypeMock:
            config = new MockConfiguration(name);
            break;
#endif
    }
    return config;
}

/*!
  Duplicate link settings
  @return A new copy of the given settings instance
*/
LinkConfiguration* LinkConfiguration::duplicateSettings(LinkConfiguration* source)
{
    LinkConfiguration* dupe = NULL;
    switch(source->type()) {
#ifndef __ios__
        case TypeSerial:
            dupe = new SerialConfiguration(dynamic_cast<SerialConfiguration*>(source));
            break;
#endif
        case TypeUdp:
            dupe = new UDPConfiguration(dynamic_cast<UDPConfiguration*>(source));
            break;
        case TypeTcp:
            dupe = new TCPConfiguration(dynamic_cast<TCPConfiguration*>(source));
            break;
        case TypeLogReplay:
            dupe = new LogReplayLinkConfiguration(dynamic_cast<LogReplayLinkConfiguration*>(source));
            break;
#ifdef QT_DEBUG
        case TypeMock:
            dupe = new MockConfiguration(dynamic_cast<MockConfiguration*>(source));
            break;
#endif
    }
    return dupe;
}
