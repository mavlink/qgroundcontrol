/**
 ******************************************************************************
 * @file
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @see        The GNU Public License (GPL) Version 3
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup importexportplugin
 * @{
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
#ifndef XMLCONFIG_H
#define XMLCONFIG_H

#if defined(QTCREATOR_UTILS_LIB)
#  define XMLCONFIG_EXPORT Q_DECL_EXPORT
#else
#  define XMLCONFIG_EXPORT Q_DECL_IMPORT
#endif

#include <QtCore/qglobal.h>
#include <QSettings>
#include <QDomElement>
#include <QObject>

class XMLCONFIG_EXPORT XmlConfig : QObject
{

public:
    static const QSettings::Format XmlSettingsFormat;

    static bool readXmlFile(QIODevice &device, QSettings::SettingsMap &map);
    static bool writeXmlFile(QIODevice &device, const QSettings::SettingsMap &map);

private:
    static QString rootName;

    static void handleNode(QDomElement* node, QSettings::SettingsMap &map, QString path = "");
    static QSettings::SettingsMap settingsToMap(QSettings& qs);
    static QString variantToString(const QVariant &v);
    static QVariant stringToVariant(const QString &s);
    static QStringList splitArgs(const QString &s, int idx);
};

#endif // XMLCONFIG_H
/**
 * @}
 * @}
 */
