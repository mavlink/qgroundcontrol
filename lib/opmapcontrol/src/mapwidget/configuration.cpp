/**
******************************************************************************
*
* @file       configuration.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      A class that centralizes most of the mapcontrol configurations
* @see        The GNU Public License (GPL) Version 3
* @defgroup   OPMapWidget
* @{
*
*****************************************************************************/
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "configuration.h"
namespace mapcontrol
{
Configuration::Configuration()
{
    EmptytileBrush = Qt::cyan;
    MissingDataFont =QFont ("Times",10,QFont::Bold);
    EmptyTileText = "We are sorry, but we don't\nhave imagery at this zoom\nlevel for this region.";
    EmptyTileBorders = QPen(Qt::white);
    ScalePen = QPen(Qt::blue);
    SelectionPen = QPen(Qt::blue);
    DragButton = Qt::LeftButton;
}
void Configuration::SetAccessMode(core::AccessMode::Types const& type)
{
    core::OPMaps::Instance()->setAccessMode(type);
}
core::AccessMode::Types Configuration::AccessMode()
{
    return core::OPMaps::Instance()->GetAccessMode();
}
void Configuration::SetLanguage(core::LanguageType::Types const& type)
{
    core::OPMaps::Instance()->setLanguage(type);
}
core::LanguageType::Types Configuration::Language()
{
    return core::OPMaps::Instance()->GetLanguage();
}
void Configuration::SetUseMemoryCache(bool const& value)
{
    core::OPMaps::Instance()->setUseMemoryCache(value);
}
}
