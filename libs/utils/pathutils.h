/**
 ******************************************************************************
 *
 * @file       pathutils.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @brief
 * @see        The GNU Public License (GPL) Version 3
 * @defgroup
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

#ifndef PATHUTILS_H
#define PATHUTILS_H

#include "utils_global.h"
#ifndef EXTERNAL_USE
#include "../extensionsystem/pluginmanager.h"
#endif
#include <QDir>
#include <QApplication>
#include <QSettings>

namespace Utils {

class QTCREATOR_UTILS_EXPORT PathUtils
{
public:
    PathUtils();
    QString GetDataPath();
    QString RemoveDataPath(QString path);
    QString InsertDataPath(QString path);

    QString GetStoragePath();
    QString RemoveStoragePath(QString path);
    QString InsertStoragePath(QString path);

};

}

#endif /* PATHUTILS_H */
