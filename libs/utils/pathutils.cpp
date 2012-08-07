/**
 ******************************************************************************
 *
 * @file       pathutils.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Utilities to find the location of openpilot GCS files:
 *             - Plugins Share directory path
 *
 * @see        The GNU Public License (GPL) Version 3
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

#include "pathutils.h"
#include "xmlconfig.h"
#include <stdint.h>
#include <QDebug>


namespace Utils {

    PathUtils::PathUtils()
    {

    }

    /**
      Returns the base path of the share directory.

      Path is in Qt/Unix conventions, separated by "/".
      */
QString PathUtils::GetDataPath()
{
    // This routine works with "/" as the standard:
    // Figure out root:  Up one from 'bin'
    QDir rootDir = QApplication::applicationDirPath();
    rootDir.cdUp();
    const QString rootDirPath = rootDir.canonicalPath();
    QString dataPath = rootDirPath;
    dataPath += QLatin1Char('/');
    // FIXME XXX
#ifdef EXTERNAL_USE
    dataPath += QLatin1String("data");
#else
    dataPath += QLatin1String(GCS_DATA_BASENAME);
#endif
    dataPath += QLatin1Char('/');
   return dataPath;
}

/**
  Cuts the standard data path from the 'path' argument. If path does not start
with the standard data path, then do nothing.

   Always returns a path converted to "/".
  */
QString PathUtils::RemoveDataPath(QString path)
{
    // Depending on the platform, we might get either "/" or "\"
    // so we need to go to the standard ("/")
    QString goodPath = QDir::fromNativeSeparators(path);
    if (goodPath.startsWith(GetDataPath())) {
        int i = goodPath.length()- GetDataPath().length();
        return QString("%%DATAPATH%%") + goodPath.right(i);
    } else
        return goodPath;
}

/**
  Inserts the data path (only if the path starts with %%DATAPATH%%)

  Returns a "/" or "\" separated path depending on platform conventions.
  */
QString PathUtils::InsertDataPath(QString path)
{
    if (path.startsWith(QString("%%DATAPATH%%")))
    {
        QString newPath = GetDataPath();
        newPath += path.right(path.length()-12);
        return QDir::toNativeSeparators(newPath);
    }
    return QDir::toNativeSeparators(path);
}

/**
  Gets a standard user-writable location for the system
  */
QString PathUtils::GetStoragePath()
{
    // This routine works with "/" as the standard:
    // Work out where the settings are stored on the machine
    QSettings set(XmlConfig::XmlSettingsFormat, QSettings::UserScope,QLatin1String("OpenPilot"), QLatin1String("OpenPilotGCS"));
    QFileInfo f(set.fileName());
    QDir dir(f.absoluteDir());

    const QString homeDirPath = dir.canonicalPath();
    QString storagePath = homeDirPath;
    storagePath += QLatin1Char('/');
    // storagePath += QLatin1String("OpenPilot");
    // storagePath += QLatin1Char('/');
   return storagePath;
}

/**
  Removes the standard storage path and replace with a tag
  */
QString PathUtils::RemoveStoragePath(QString path)
{
    // Depending on the platform, we might get either "/" or "\"
    // so we need to go to the standard ("/")
    QString goodPath = QDir::fromNativeSeparators(path);
    if (goodPath.startsWith(GetStoragePath())) {
        int i = goodPath.length()- GetStoragePath().length();
        return QString("%%STOREPATH%%") + goodPath.right(i);
    } else
        return goodPath;
}

/**
  Inserts the standard storage path is there is a storage path tag
  */
QString PathUtils::InsertStoragePath(QString path)
{
    if (path.startsWith(QString("%%STOREPATH%%")))
    {
        QString newPath = GetStoragePath();
        newPath += path.right(path.length()-13);
        return QDir::toNativeSeparators(newPath);
    }
    return QDir::toNativeSeparators(path);

}

}
