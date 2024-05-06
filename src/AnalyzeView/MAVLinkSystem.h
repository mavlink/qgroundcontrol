/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
/// @brief MAVLink message inspector and charting controller
/// @author Gus Grubba <gus@auterion.com>

#pragma once

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QLoggingCategory>
#include <QtQmlIntegration/QtQmlIntegration>

#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(MAVLinkSystemLog)

class QGCMAVLinkMessage;

//-----------------------------------------------------------------------------
/// Vehicle MAVLink message belongs to
class QGCMAVLinkSystem : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    Q_PROPERTY(quint8               id              READ id                                 CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  messages        READ messages                           CONSTANT)
    Q_PROPERTY(QList<int>           compIDs         READ compIDs                            NOTIFY compIDsChanged)
    Q_PROPERTY(QStringList          compIDsStr      READ compIDsStr                         NOTIFY compIDsChanged)
    Q_PROPERTY(int                  selected        READ selected       WRITE setSelected   NOTIFY selectedChanged)

    QGCMAVLinkSystem   (QObject* parent, quint8 id);
    ~QGCMAVLinkSystem  ();

    quint8              id              () const{ return _id; }
    QmlObjectListModel* messages        () { return &_messages; }
    QList<int>          compIDs         () { return _compIDs; }
    QStringList         compIDsStr      () { return _compIDsStr; }
    int                 selected        () const{ return _selected; }

    void                setSelected     (int sel);
    QGCMAVLinkMessage*  findMessage     (uint32_t id, uint8_t compId);
    int                 findMessage     (QGCMAVLinkMessage* message);
    void                append          (QGCMAVLinkMessage* message);
    QGCMAVLinkMessage*  selectedMsg     ();

signals:
    void compIDsChanged                 ();
    void selectedChanged                ();

private:
    void _checkCompID                   (QGCMAVLinkMessage *message);
    void _resetSelection                ();

private:
    quint8              _id;
    QList<int>          _compIDs;
    QStringList         _compIDsStr;
    QmlObjectListModel  _messages;      //-- List of QGCMAVLinkMessage
    int                 _selected = 0;
};
