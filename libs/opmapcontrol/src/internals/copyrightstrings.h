/**
******************************************************************************
*
* @file       copyrightstrings.h
* @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
* @brief      
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
#ifndef COPYRIGHTSTRINGS_H
#define COPYRIGHTSTRINGS_H

#include <QString>
#include <QDateTime>
 
namespace internals {
static const QString googleCopyright = QString("©%1 Google - Map data ©%1 Tele Atlas, Imagery ©%1 TerraMetrics").arg(QDate::currentDate().year());
static const QString openStreetMapCopyright = QString("© OpenStreetMap - Map data ©%1 OpenStreetMap").arg(QDate::currentDate().year());
static const QString yahooMapCopyright = QString("© Yahoo! Inc. - Map data & Imagery ©%1 NAVTEQ").arg(QDate::currentDate().year());
static const QString virtualEarthCopyright = QString("©%1 Microsoft Corporation, ©%1 NAVTEQ, ©%1 Image courtesy of NASA").arg(QDate::currentDate().year());
static const QString arcGisCopyright = QString("©%1 ESRI - Map data ©%1 ArcGIS").arg(QDate::currentDate().year());

}
#endif // COPYRIGHTSTRINGS_H
