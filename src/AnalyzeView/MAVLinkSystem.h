/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "QmlObjectListModel.h"

Q_DECLARE_LOGGING_CATEGORY(MAVLinkSystemLog)

class QGCMAVLinkMessage;

class QGCMAVLinkSystem : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    Q_PROPERTY(quint8               id          READ id                             CONSTANT)
    Q_PROPERTY(QmlObjectListModel   *messages   READ messages                       CONSTANT)
    Q_PROPERTY(QList<int>           compIDs     READ compIDs                        NOTIFY compIDsChanged)
    Q_PROPERTY(QStringList          compIDsStr  READ compIDsStr                     NOTIFY compIDsChanged)
    Q_PROPERTY(int                  selected    READ selected   WRITE setSelected   NOTIFY selectedChanged)
public:
    QGCMAVLinkSystem(quint8 id, QObject *parent = nullptr);
    ~QGCMAVLinkSystem();

    quint8 id() const { return _id; }
    QmlObjectListModel *messages() const { return _messages; }
    QList<int> compIDs() const { return _compIDs; }
    QStringList compIDsStr() const { return _compIDsStr; }
    int selected() const { return _selected; }

    void setSelected(int sel);
    QGCMAVLinkMessage *findMessage(uint32_t id, uint8_t compId);
    int findMessage(const QGCMAVLinkMessage *message);
    void append(QGCMAVLinkMessage *message);
    QGCMAVLinkMessage *selectedMsg();

signals:
    void compIDsChanged();
    void selectedChanged();

private:
    void _checkCompID(const QGCMAVLinkMessage *message);
    void _resetSelection();

private:
    quint8 _id = 0;
    QmlObjectListModel *_messages = nullptr; ///< List of QGCMAVLinkMessage
    QList<int> _compIDs;
    QStringList _compIDsStr;
    int _selected = 0;
};
