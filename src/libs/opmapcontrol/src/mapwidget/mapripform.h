/**
******************************************************************************
*
* @file       mapripform.h
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
#ifndef MAPRIPFORM_H
#define MAPRIPFORM_H

#include <QWidget>

namespace Ui {
    class MapRipForm;
}

class MapRipForm : public QWidget
{
    Q_OBJECT

public:
    explicit MapRipForm(QWidget *parent = 0);
    ~MapRipForm();
public slots:
    void SetPercentage(int const& perc);
    void SetProvider(QString const& prov,int const& zoom);
    void SetNumberOfTiles(int const& total,int const& actual);
private:
    Ui::MapRipForm *ui;
};

#endif // MAPRIPFORM_H
