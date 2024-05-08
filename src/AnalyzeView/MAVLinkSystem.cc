/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkSystem.h"
#include "MAVLinkMessage.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MAVLinkSystemLog, "qgc.analyzeview.mavlinksystem")

//-----------------------------------------------------------------------------
QGCMAVLinkSystem::QGCMAVLinkSystem(QObject* parent, quint8 id)
    : QObject(parent)
    , _id(id)
{
    qCDebug(MAVLinkSystemLog) << "New Vehicle:" << id;
}

//-----------------------------------------------------------------------------
QGCMAVLinkSystem::~QGCMAVLinkSystem()
{
    _messages.clearAndDeleteContents();
}

//-----------------------------------------------------------------------------
QGCMAVLinkMessage*
QGCMAVLinkSystem::findMessage(uint32_t id, uint8_t compId)
{
    for(int i = 0; i < _messages.count(); i++) {
        QGCMAVLinkMessage* m = qobject_cast<QGCMAVLinkMessage*>(_messages.get(i));
        if(m) {
            if(m->id() == id && m->compId() == compId) {
                return m;
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
int
QGCMAVLinkSystem::findMessage(QGCMAVLinkMessage* message)
{
    for(int i = 0; i < _messages.count(); i++) {
        QGCMAVLinkMessage* m = qobject_cast<QGCMAVLinkMessage*>(_messages.get(i));
        if(m && m == message) {
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkSystem::_resetSelection()
{
    for(int i = 0; i < _messages.count(); i++) {
        QGCMAVLinkMessage* m = qobject_cast<QGCMAVLinkMessage*>(_messages.get(i));
        if(m && m->selected()) {
            m->setSelected(false);
            emit m->selectedChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkSystem::setSelected(int sel)
{
    if(sel < _messages.count()) {
        _selected = sel;
        emit selectedChanged();
        _resetSelection();
        QGCMAVLinkMessage* m = qobject_cast<QGCMAVLinkMessage*>(_messages.get(sel));
        if(m && !m->selected()) {
            m->setSelected(true);
            emit m->selectedChanged();
        }
    }
}

QGCMAVLinkMessage* QGCMAVLinkSystem::selectedMsg()
{
    QGCMAVLinkMessage* selectedMsg = nullptr;
    if(_messages.count())
    {
        selectedMsg = qobject_cast<QGCMAVLinkMessage*>(_messages.get(_selected));
    }
    return selectedMsg;
}

//-----------------------------------------------------------------------------
static bool
messages_sort(QObject* a, QObject* b)
{
    QGCMAVLinkMessage* aa = qobject_cast<QGCMAVLinkMessage*>(a);
    QGCMAVLinkMessage* bb = qobject_cast<QGCMAVLinkMessage*>(b);
    if(!aa || !bb) return false;
    if(aa->name() == bb->name()) return aa->name() < bb->name();
    return aa->name() < bb->name();
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkSystem::append(QGCMAVLinkMessage* message)
{
    //-- Save selected message
    QGCMAVLinkMessage* selectedMsg = nullptr;
    if(_messages.count()) {
        selectedMsg = qobject_cast<QGCMAVLinkMessage*>(_messages.get(_selected));
    } else {
        //-- First message
        message->setSelected(true);
    }
    _messages.append(message);
    //-- Sort messages by id and then compId
    if (_messages.count() > 0) {
        _messages.beginReset();
        std::sort(_messages.objectList()->begin(), _messages.objectList()->end(), messages_sort);
        _messages.endReset();
        _checkCompID(message);
    }
    //-- Remember selected message
    if(selectedMsg) {
        int idx = findMessage(selectedMsg);
        if(idx >= 0) {
            _selected = idx;
            emit selectedChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkSystem::_checkCompID(QGCMAVLinkMessage* message)
{
    if(_compIDsStr.isEmpty()) {
        _compIDsStr << tr("Comp All");
    }
    if(!_compIDs.contains(static_cast<int>(message->compId()))) {
        int compId = static_cast<int>(message->compId());
        _compIDs.append(compId);
        _compIDsStr << tr("Comp %1").arg(compId);
        emit compIDsChanged();
    }
}
