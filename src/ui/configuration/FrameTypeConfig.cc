/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2013 Michael Carpenter (malcom2073@gmail.com)

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

/**
 * @file
 *   @brief Airframe type configuration widget source.
 *
 *   @author Michael Carpenter <malcom2073@gmail.com>
 *
 */

#include "FrameTypeConfig.h"


FrameTypeConfig::FrameTypeConfig(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    connect(ui.plusRadioButton,SIGNAL(clicked()),this,SLOT(plusFrameSelected()));
    connect(ui.xRadioButton,SIGNAL(clicked()),this,SLOT(xFrameSelected()));
    connect(ui.vRadioButton,SIGNAL(clicked()),this,SLOT(vFrameSelected()));
    //connect(UASManager::instance()->getActiveUAS()->getParamManager(),SIGNAL(parameterListUpToDate(int))
}

FrameTypeConfig::~FrameTypeConfig()
{
}
void FrameTypeConfig::xFrameSelected()
{

}

void FrameTypeConfig::plusFrameSelected()
{

}

void FrameTypeConfig::vFrameSelected()
{

}
