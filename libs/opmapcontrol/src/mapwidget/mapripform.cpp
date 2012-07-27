/**
******************************************************************************
*
* @file       mapripform.cpp
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      Form to be used with the MapRipper class
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

#include "mapripform.h"
#include "ui_mapripform.h"

MapRipForm::MapRipForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MapRipForm)
{
    ui->setupUi(this);
}

MapRipForm::~MapRipForm()
{
    delete ui;
}
void MapRipForm::SetPercentage(const int &perc)
{
    ui->progressBar->setValue(perc);
}
void MapRipForm::SetProvider(const QString &prov,const int &zoom)
{
    ui->mainlabel->setText(QString("Currently ripping from:%1 at Zoom level %2").arg(prov).arg(zoom));
}
void MapRipForm::SetNumberOfTiles(const int &total, const int &actual)
{
    ui->statuslabel->setText(QString("Downloading tile %1 of %2").arg(actual).arg(total));
}
