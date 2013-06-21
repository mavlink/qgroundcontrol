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
    m_uas=0;
    ui.setupUi(this);

    //Disable until we get a FRAME parameter.
    ui.xRadioButton->setEnabled(false);
    ui.vRadioButton->setEnabled(false);
    ui.plusRadioButton->setEnabled(false);

    connect(ui.plusRadioButton,SIGNAL(clicked()),this,SLOT(plusFrameSelected()));
    connect(ui.xRadioButton,SIGNAL(clicked()),this,SLOT(xFrameSelected()));
    connect(ui.vRadioButton,SIGNAL(clicked()),this,SLOT(vFrameSelected()));
    connect(UASManager::instance(),SIGNAL(activeUASSet(UASInterface*)),this,SLOT(activeUASSet(UASInterface*)));
    activeUASSet(UASManager::instance()->getActiveUAS());
}

FrameTypeConfig::~FrameTypeConfig()
{
}
void FrameTypeConfig::activeUASSet(UASInterface *uas)
{
    if (!uas) return;
    if (!m_uas)
    {
        disconnect(m_uas,SIGNAL(parameterChanged(int,int,QString,QVariant)),this,SLOT(parameterChanged(int,int,QString,QVariant)));
    }
    m_uas = uas;
    connect(m_uas,SIGNAL(parameterChanged(int,int,QString,QVariant)),this,SLOT(parameterChanged(int,int,QString,QVariant)));
}
void FrameTypeConfig::parameterChanged(int uas, int component, QString parameterName, QVariant value)
{
    if (parameterName == "FRAME")
    {
        ui.xRadioButton->setEnabled(true);
        ui.vRadioButton->setEnabled(true);
        ui.plusRadioButton->setEnabled(true);
        if (value.toInt() == 0)
        {
            ui.plusRadioButton->setChecked(true);
        }
        else if (value.toInt() == 1)
        {
            ui.xRadioButton->setChecked(true);
        }
        else if (value.toInt() == 2)
        {
            ui.vRadioButton->setChecked(true);
        }
    }
}

void FrameTypeConfig::xFrameSelected()
{
    if (m_uas)
    {
        m_uas->setParameter(0,"FRAME",QVariant(1));
    }
}

void FrameTypeConfig::plusFrameSelected()
{
    if (m_uas)
    {
        m_uas->setParameter(0,"FRAME",QVariant(0));
    }
}

void FrameTypeConfig::vFrameSelected()
{
    if (m_uas)
    {
        m_uas->setParameter(0,"FRAME",QVariant(2));
    }
}
