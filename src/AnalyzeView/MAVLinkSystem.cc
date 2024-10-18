/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkSystem.h"
#include "MAVLinkMessage.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

QGC_LOGGING_CATEGORY(MAVLinkSystemLog, "qgc.analyzeview.mavlinksystem")

QGCMAVLinkSystem::QGCMAVLinkSystem(quint8 id, QObject *parent)
    : QObject(parent)
    , _id(id)
    , _messages(new QmlObjectListModel(this))
{
    // qCDebug(MAVLinkSystemLog) << Q_FUNC_INFO << this;

    qCDebug(MAVLinkSystemLog) << "New Vehicle:" << id;
}

QGCMAVLinkSystem::~QGCMAVLinkSystem()
{
    _messages->clearAndDeleteContents();

    // qCDebug(MAVLinkSystemLog) << Q_FUNC_INFO << this;
}

QGCMAVLinkMessage *QGCMAVLinkSystem::findMessage(uint32_t id, uint8_t compId) const
{
    for (int i = 0; i < _messages->count(); i++) {
        QGCMAVLinkMessage *const msg = qobject_cast<QGCMAVLinkMessage*>(_messages->get(i));
        if (msg && (msg->id() == id) && (msg->compId() == compId)) {
            return msg;
        }
    }

    return nullptr;
}

int QGCMAVLinkSystem::findMessage(QGCMAVLinkMessage *message) const
{
    for (int i = 0; i < _messages->count(); i++) {
        QGCMAVLinkMessage *const msg = qobject_cast<QGCMAVLinkMessage*>(_messages->get(i));
        if(msg && (msg == message)) {
            return i;
        }
    }

    return -1;
}

void QGCMAVLinkSystem::_resetSelection()
{
    for (int i = 0; i < _messages->count(); i++) {
        QGCMAVLinkMessage *const msg = qobject_cast<QGCMAVLinkMessage*>(_messages->get(i));
        if (msg && msg->selected()) {
            msg->setSelected(false);
            emit msg->selectedChanged();
        }
    }
}

void QGCMAVLinkSystem::setSelected(int sel)
{
    if (sel >= _messages->count()) {
        return;
    }

    _selected = sel;
    emit selectedChanged();

    _resetSelection();

    QGCMAVLinkMessage *const msg = qobject_cast<QGCMAVLinkMessage*>(_messages->get(sel));
    if (msg && !msg->selected()) {
        msg->setSelected(true);
        emit msg->selectedChanged();
    }
}

QGCMAVLinkMessage *QGCMAVLinkSystem::selectedMsg()
{
    return (_messages->count() ? qobject_cast<QGCMAVLinkMessage*>(_messages->get(_selected)) : nullptr);
}

bool QGCMAVLinkSystem::_messagesSort(const QObject *a, const QObject *b)
{
    const QGCMAVLinkMessage *const aa = qobject_cast<const QGCMAVLinkMessage*>(a);
    const QGCMAVLinkMessage *const bb = qobject_cast<const QGCMAVLinkMessage*>(b);

    if (!aa || !bb) {
        return false;
    }

    if (aa->name() == bb->name()) {
        return (aa->name() < bb->name());
    }

    return (aa->name() < bb->name());
}

void QGCMAVLinkSystem::append(QGCMAVLinkMessage *message)
{
    QGCMAVLinkMessage *selectedMsg = nullptr;

    if(_messages->count()) {
        selectedMsg = qobject_cast<QGCMAVLinkMessage*>(_messages->get(_selected));
    } else {
        message->setSelected(true);
    }

    (void) _messages->append(message);

    //-- Sort messages by id and then compId
    if (_messages->count() > 0) {
        _messages->beginReset();
        std::sort(_messages->objectList()->begin(), _messages->objectList()->end(), _messagesSort);
        _messages->endReset();
        _checkCompID(message);
    }

    //-- Remember selected message
    if (selectedMsg) {
        const int idx = findMessage(selectedMsg);
        if (idx >= 0) {
            _selected = idx;
            emit selectedChanged();
        }
    }
}

void QGCMAVLinkSystem::_checkCompID(QGCMAVLinkMessage *message)
{
    if (_compIDsStr.isEmpty()) {
        _compIDsStr << tr("Comp All");
    }

    if (!_compIDs.contains(static_cast<int>(message->compId()))) {
        const int compId = static_cast<int>(message->compId());
        (void) _compIDs.append(compId);
        _compIDsStr << tr("Comp %1").arg(compId);
        emit compIDsChanged();
    }
}
