/**
 ******************************************************************************
 *
 * @file       xmlconfig.cpp
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @see        The GNU Public License (GPL) Version 3
 * @brief      Widget for Import/Export Plugin
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup   importexportplugin
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
/* Nokia Corporation */
#include "xmlconfig.h"

#include <QtDebug>
#include <QStringList>
#include <QRegExp>

#include <QVariant>
#include <QRect>
#include <QSize>
#include <QPoint>
#include <QtCore/QUrl>

#define NUM_PREFIX "arr_"

QString XmlConfig::rootName = "gcs";

const QSettings::Format XmlConfig::XmlSettingsFormat =
        QSettings::registerFormat("xml", XmlConfig::readXmlFile, XmlConfig::writeXmlFile);


bool XmlConfig::readXmlFile(QIODevice &device, QSettings::SettingsMap &map)
{
    QDomDocument domDoc;
    QDomElement root;
    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!domDoc.setContent(&device, true, &errorStr, &errorLine,
                                &errorColumn)) {
        QString err = QString(tr("GCS config")) +
                      tr("Parse error at line %1, column %2:\n%3")
                      .arg(errorLine)
                      .arg(errorColumn)
                      .arg(errorStr);
        qFatal("%s", err.toLatin1().data());
        return false;
    }
    root = domDoc.documentElement();
    handleNode(&root, map);

    return true;
}

void XmlConfig::handleNode(QDomElement* node, QSettings::SettingsMap &map, QString path)
{
    if ( !node ){
        return;
    }
  //  qDebug() << "XmlConfig::handleNode start";

    QString nodeName = node->nodeName();
    // For arrays, QT will use simple numbers as keys, which is not a valid element in XML.
    // Therefore we prefixed these.
    if ( nodeName.startsWith(NUM_PREFIX) ){
        nodeName.replace(NUM_PREFIX, "");
    }
    // Xml tags are restrictive with allowed characters,
    // so we urlencode and replace % with __PCT__ on file
    nodeName = nodeName.replace("__PCT__", "%");
    nodeName = QUrl::fromPercentEncoding(nodeName.toAscii());

    if ( nodeName == XmlConfig::rootName )
        ;
    else if ( path == "" )
        path = nodeName;
    else
        path += "/" + nodeName;

//    qDebug() << "Node: " << ": " << path << " Children: " << node->childNodes().length();
    for ( uint i = 0; i < node->childNodes().length(); ++i ){
        QDomNode child = node->childNodes().item(i);
        if ( child.isElement() ){
            handleNode( static_cast<QDomElement*>(&child), map, path);
        }
        else if ( child.isText() ){
//            qDebug() << "Key: " << path << " Value:" << node->text();
            map.insert(path, stringToVariant(node->text()));
        }
        else{
            qDebug() << "Child not Element or text!" << child.nodeType();
        }
    }
//    qDebug() << "XmlConfig::handleNode end";
}

bool XmlConfig::writeXmlFile(QIODevice &device, const QSettings::SettingsMap &map)
{
    QDomDocument outDocument;
//    qDebug() << "writeXmlFile start";
    outDocument.appendChild( outDocument.createElement(XmlConfig::rootName));
    QMapIterator<QString, QVariant> iter(map);
    while (iter.hasNext()) {
        iter.next();
//        qDebug() << "Entry: " << iter.key() << ": " << iter.value().toString() << endl;
        QDomNode node = outDocument.firstChild();
        foreach ( QString elem, iter.key().split('/')){
            if ( elem == "" ){
                continue;
            }
            // Xml tags are restrictive with allowed characters,
            // so we urlencode and replace % with __PCT__ on file
            elem = QString(QUrl::toPercentEncoding(elem));
            elem = elem.replace("%", "__PCT__");
            // For arrays, QT will use simple numbers as keys, which is not a valid element in XML.
            // Therefore we prefixed these.
            if ( elem.startsWith(NUM_PREFIX) ){
                qWarning() << "ERROR: Settings must not start with " << NUM_PREFIX
                        << " in: " + iter.key();
            }
            if ( QRegExp("[0-9]").exactMatch(elem.left(1)) ){
                elem.prepend(NUM_PREFIX);
            }
            if ( node.firstChildElement(elem).isNull() ){
                node.appendChild(outDocument.createElement(elem));
            }
            node = node.firstChildElement(elem);
        }
        node.appendChild(outDocument.createTextNode(variantToString(iter.value())));
    }
    device.write(outDocument.toByteArray(2).constData());
//    qDebug() << "Dokument:\n" << outDocument.toByteArray(2).constData();
//    qDebug() << "writeXmlFile end";
    return true;
}


QSettings::SettingsMap XmlConfig::settingsToMap(QSettings& qs){
    qDebug() << "settingsToMap:---------------";
    QSettings::SettingsMap map;
    QStringList keys = qs.allKeys();
    foreach (QString key, keys) {
        QVariant val = qs.value(key);
        qDebug() << key << val.toString();
        map.insert(key, val);
    }
    qDebug() << "settingsToMap End --------";
    return map;
}

QString XmlConfig::variantToString(const QVariant &v)
{
    QString result;

    switch (v.type()) {
        case QVariant::Invalid:
            result = QLatin1String("@Invalid()");
            break;

        case QVariant::ByteArray: {
            QByteArray a = v.toByteArray().toBase64();
            result = QLatin1String("@ByteArray(");
            result += QString::fromLatin1(a.constData(), a.size());
            result += QLatin1Char(')');
            break;
        }

        case QVariant::String:
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::Bool:
        case QVariant::Double:
        case QVariant::KeySequence:
        case QVariant::Color: {
            result = v.toString();
            if (result.startsWith(QLatin1Char('@')))
                result.prepend(QLatin1Char('@'));
            break;
        }
#ifndef QT_NO_GEOM_VARIANT
    case QVariant::Rect: {
        QRect r = qvariant_cast<QRect>(v);
        result += QLatin1String("@Rect(");
        result += QString::number(r.x());
        result += QLatin1Char(' ');
        result += QString::number(r.y());
        result += QLatin1Char(' ');
        result += QString::number(r.width());
        result += QLatin1Char(' ');
        result += QString::number(r.height());
        result += QLatin1Char(')');
        break;
    }
    case QVariant::Size: {
        QSize s = qvariant_cast<QSize>(v);
        result += QLatin1String("@Size(");
        result += QString::number(s.width());
        result += QLatin1Char(' ');
        result += QString::number(s.height());
        result += QLatin1Char(')');
        break;
    }
    case QVariant::Point: {
        QPoint p = qvariant_cast<QPoint>(v);
        result += QLatin1String("@Point(");
        result += QString::number(p.x());
        result += QLatin1Char(' ');
        result += QString::number(p.y());
        result += QLatin1Char(')');
        break;
    }
#endif // !QT_NO_GEOM_VARIANT

    default: {
#ifndef QT_NO_DATASTREAM
        QByteArray a;
        {
            QDataStream s(&a, QIODevice::WriteOnly);
            s.setVersion(QDataStream::Qt_4_0);
            s << v;
        }

        result = QLatin1String("@Variant(");
        result += QString::fromLatin1(a.toBase64().constData());
        result += QLatin1Char(')');
	// These were being much too noisy!!
        //qDebug() << "Variant Type: " << v.type();
        //qDebug()<< "Variant: " << result;
#else
        Q_ASSERT(!"QSettings: Cannot save custom types without QDataStream support");
#endif
        break;
    }
}

return result;
}

QVariant XmlConfig::stringToVariant(const QString &s)
{
    if (s.startsWith(QLatin1Char('@'))) {
        if (s.endsWith(QLatin1Char(')'))) {
            if (s.startsWith(QLatin1String("@ByteArray("))) {
                return QVariant(QByteArray::fromBase64(s.toLatin1().mid(11, s.size() - 12)));
            } else if (s.startsWith(QLatin1String("@Variant("))) {
#ifndef QT_NO_DATASTREAM
                QByteArray a(QByteArray::fromBase64(s.toLatin1().mid(9)));
                QDataStream stream(&a, QIODevice::ReadOnly);
                stream.setVersion(QDataStream::Qt_4_0);
                QVariant result;
                stream >> result;
                return result;
#else
                Q_ASSERT(!"QSettings: Cannot load custom types without QDataStream support");
#endif
#ifndef QT_NO_GEOM_VARIANT
            } else if (s.startsWith(QLatin1String("@Rect("))) {
                QStringList args = splitArgs(s, 5);
                if (args.size() == 4)
                    return QVariant(QRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt()));
            } else if (s.startsWith(QLatin1String("@Size("))) {
                QStringList args = splitArgs(s, 5);
                if (args.size() == 2)
                    return QVariant(QSize(args[0].toInt(), args[1].toInt()));
            } else if (s.startsWith(QLatin1String("@Point("))) {
                QStringList args = splitArgs(s, 6);
                if (args.size() == 2)
                    return QVariant(QPoint(args[0].toInt(), args[1].toInt()));
#endif
            } else if (s == QLatin1String("@Invalid()")) {
                return QVariant();
            }

        }
        if (s.startsWith(QLatin1String("@@")))
            return QVariant(s.mid(1));
    }

    return QVariant(s);
}

QStringList XmlConfig::splitArgs(const QString &s, int idx)
{
    int l = s.length();
    Q_ASSERT(l > 0);
    Q_ASSERT(s.at(idx) == QLatin1Char('('));
    Q_ASSERT(s.at(l - 1) == QLatin1Char(')'));

    QStringList result;
    QString item;

    for (++idx; idx < l; ++idx) {
        QChar c = s.at(idx);
        if (c == QLatin1Char(')')) {
            Q_ASSERT(idx == l - 1);
            result.append(item);
        } else if (c == QLatin1Char(' ')) {
            result.append(item);
            item.clear();
        } else {
            item.append(c);
        }
    }

    return result;
}

/**
 * @}
 * @}
 */
