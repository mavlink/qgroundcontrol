/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkInspectorController.h"
#include "QGCApplication.h"
#include "MultiVehicleManager.h"
#include <QtCharts/QLineSeries>

QGC_LOGGING_CATEGORY(MAVLinkInspectorLog, "MAVLinkInspectorLog")

QT_CHARTS_USE_NAMESPACE

Q_DECLARE_METATYPE(QAbstractSeries*)

//-----------------------------------------------------------------------------
QGCMAVLinkMessageField::QGCMAVLinkMessageField(QGCMAVLinkMessage *parent, QString name, QString type)
    : QObject(parent)
    , _type(type)
    , _name(name)
    , _msg(parent)
{
    qCDebug(MAVLinkInspectorLog) << "Field:" << name << type;
}

//-----------------------------------------------------------------------------
QString
QGCMAVLinkMessageField::label()
{
    return QString(_msg->name() + ": " + _name);
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessageField::setSelectable(bool sel)
{
    if(_selectable != sel) {
        _selectable = sel;
        emit selectableChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessageField::setSelected(bool sel)
{
    if(_selected != sel) {
        _selected = sel;
        emit selectedChanged();
        _values.clear();
        _times.clear();
        _rangeMin  = 0;
        _rangeMax  = 0;
        _dataIndex = 0;
        emit rangeMinChanged();
        emit rangeMaxChanged();
        if(_selected) {
            _msg->msgCtl()->addChartField(this);
        } else {
            _msg->msgCtl()->delChartField(this);
        }
        _msg->select();
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessageField::updateValue(QString newValue, qreal v)
{
    if(_value != newValue) {
        _value = newValue;
        emit valueChanged();
    }
    if(_selected) {
        int count = _values.count();
        //-- Arbitrary limit of 1 minute of data at 50Hz for now
        if(count < (50 * 60)) {
            _values.append(v);
            _times.append(QGC::groundTimeMilliseconds());
        } else {
            if(_dataIndex >= count) _dataIndex = 0;
            _values[_dataIndex] = v;
            _times[_dataIndex]  = QGC::groundTimeMilliseconds();
            _dataIndex++;
        }
        qreal vmin  = std::numeric_limits<qreal>::max();
        qreal vmax  = std::numeric_limits<qreal>::min();
        for(int i = 0; i < _values.count(); i++) {
            qreal v = _values[i];
            if(vmax < v) vmax = v;
            if(vmin > v) vmin = v;
        }
        if(std::abs(_rangeMin - vmin) > 0.000001) {
            _rangeMin = vmin;
            emit rangeMinChanged();
        }
        if(std::abs(_rangeMax - vmax) > 0.000001) {
            _rangeMax = vmax;
            emit rangeMaxChanged();
        }
        _msg->msgCtl()->updateXRange();
        _updateSeries();
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessageField::_updateSeries()
{
    int count = _values.count();
    if (count > 1) {
        _series.clear();
        int idx = _dataIndex;
        for(int i = 0; i < count; i++, idx++) {
            if(idx >= count) idx = 0;
            QPointF p(_times[idx], _values[idx]);
            _series.append(p);
        }
        emit seriesChanged();
    }
}

//-----------------------------------------------------------------------------
QGCMAVLinkMessage::QGCMAVLinkMessage(MAVLinkInspectorController *parent, mavlink_message_t* message)
    : QObject(parent)
    , _msgCtl(parent)
{
    _message = *message;
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info(message);
    if (!msgInfo) {
        qWarning() << QStringLiteral("QGCMAVLinkMessage NULL msgInfo msgid(%1)").arg(message->msgid);
        return;
    }
    _name = QString(msgInfo->name);
    qCDebug(MAVLinkInspectorLog) << "New Message:" << _name;
    for (unsigned int i = 0; i < msgInfo->num_fields; ++i) {
        QString type = QString("?");
        switch (msgInfo->fields[i].type) {
            case MAVLINK_TYPE_CHAR:     type = QString("char");     break;
            case MAVLINK_TYPE_UINT8_T:  type = QString("uint8_t");  break;
            case MAVLINK_TYPE_INT8_T:   type = QString("int8_t");   break;
            case MAVLINK_TYPE_UINT16_T: type = QString("uint16_t"); break;
            case MAVLINK_TYPE_INT16_T:  type = QString("int16_t");  break;
            case MAVLINK_TYPE_UINT32_T: type = QString("uint32_t"); break;
            case MAVLINK_TYPE_INT32_T:  type = QString("int32_t");  break;
            case MAVLINK_TYPE_FLOAT:    type = QString("float");    break;
            case MAVLINK_TYPE_DOUBLE:   type = QString("double");   break;
            case MAVLINK_TYPE_UINT64_T: type = QString("uint64_t"); break;
            case MAVLINK_TYPE_INT64_T:  type = QString("int64_t");  break;
        }
        QGCMAVLinkMessageField* f = new QGCMAVLinkMessageField(this, msgInfo->fields[i].name, type);
        _fields.append(f);
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessage::select()
{
    bool sel = false;
    for (int i = 0; i < _fields.count(); ++i) {
        QGCMAVLinkMessageField* f = qobject_cast<QGCMAVLinkMessageField*>(_fields.get(i));
        if(f) {
            if(f->selected()) {
                sel = true;
                break;
            }
        }
    }
    if(sel != _selected) {
        _selected = sel;
        emit selectedChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessage::updateFreq()
{
    quint64 msgCount = _count - _lastCount;
    _messageHz = (0.2 * _messageHz) + (0.8 * msgCount);
    _lastCount = _count;
    emit freqChanged();
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessage::update(mavlink_message_t* message)
{
    _message = *message;
    _count++;
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info(message);
    if (!msgInfo) {
        qWarning() << QStringLiteral("QGCMAVLinkMessage::update NULL msgInfo msgid(%1)").arg(message->msgid);
        return;
    }
    if(_fields.count() != static_cast<int>(msgInfo->num_fields)) {
        qWarning() << QStringLiteral("QGCMAVLinkMessage::update msgInfo field count mismatch msgid(%1)").arg(message->msgid);
        return;
    }
    uint8_t* m = reinterpret_cast<uint8_t*>(&message->payload64[0]);
    for (unsigned int i = 0; i < msgInfo->num_fields; ++i) {
        QGCMAVLinkMessageField* f = qobject_cast<QGCMAVLinkMessageField*>(_fields.get(static_cast<int>(i)));
        if(f) {
            switch (msgInfo->fields[i].type) {
            case MAVLINK_TYPE_CHAR:
                f->setSelectable(false);
                if (msgInfo->fields[i].array_length > 0) {
                    char* str = reinterpret_cast<char*>(m+ msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    str[msgInfo->fields[i].array_length - 1] = '\0';
                    QString v(str);
                    f->updateValue(v, 0);
                } else {
                    // Single char
                    char b = *(reinterpret_cast<char*>(m + msgInfo->fields[i].wire_offset));
                    QString v(b);
                    f->updateValue(v, 0);
                }
                break;
            case MAVLINK_TYPE_UINT8_T:
                if (msgInfo->fields[i].array_length > 0) {
                    uint8_t* nums = m+msgInfo->fields[i].wire_offset;
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[msgInfo->fields[i].array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    uint8_t u = *(m+msgInfo->fields[i].wire_offset);
                    f->updateValue(QString::number(u), static_cast<qreal>(u));
                }
                break;
            case MAVLINK_TYPE_INT8_T:
                if (msgInfo->fields[i].array_length > 0) {
                    int8_t* nums = reinterpret_cast<int8_t*>(m + msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[msgInfo->fields[i].array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    int8_t n = *(reinterpret_cast<int8_t*>(m+msgInfo->fields[i].wire_offset));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            case MAVLINK_TYPE_UINT16_T:
                if (msgInfo->fields[i].array_length > 0) {
                    uint16_t* nums = reinterpret_cast<uint16_t*>(m + msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[msgInfo->fields[i].array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    uint16_t n = *(reinterpret_cast<uint16_t*>(m + msgInfo->fields[i].wire_offset));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            case MAVLINK_TYPE_INT16_T:
                if (msgInfo->fields[i].array_length > 0) {
                    int16_t* nums = reinterpret_cast<int16_t*>(m + msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[msgInfo->fields[i].array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    int16_t n = *(reinterpret_cast<int16_t*>(m + msgInfo->fields[i].wire_offset));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            case MAVLINK_TYPE_UINT32_T:
                if (msgInfo->fields[i].array_length > 0) {
                    uint32_t* nums = reinterpret_cast<uint32_t*>(m + msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[msgInfo->fields[i].array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    uint32_t n = *(reinterpret_cast<uint32_t*>(m + msgInfo->fields[i].wire_offset));
                    //-- Special case
                    if(_message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME) {
                        QDateTime d = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(n),Qt::UTC,0);
                        f->updateValue(d.toString("HH:mm:ss"), static_cast<qreal>(n));
                    } else {
                        f->updateValue(QString::number(n), static_cast<qreal>(n));
                    }
                }
                break;
            case MAVLINK_TYPE_INT32_T:
                if (msgInfo->fields[i].array_length > 0) {
                    int32_t* nums = reinterpret_cast<int32_t*>(m + msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[msgInfo->fields[i].array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    int32_t n = *(reinterpret_cast<int32_t*>(m + msgInfo->fields[i].wire_offset));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            case MAVLINK_TYPE_FLOAT:
                if (msgInfo->fields[i].array_length > 0) {
                    float* nums = reinterpret_cast<float*>(m + msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                       string += tmp.arg(static_cast<double>(nums[j]));
                    }
                    string += QString::number(static_cast<double>(nums[msgInfo->fields[i].array_length - 1]));
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    float fv = *(reinterpret_cast<float*>(m + msgInfo->fields[i].wire_offset));
                    f->updateValue(QString::number(static_cast<double>(fv)), static_cast<qreal>(fv));
                }
                break;
            case MAVLINK_TYPE_DOUBLE:
                if (msgInfo->fields[i].array_length > 0) {
                    double* nums = reinterpret_cast<double*>(m + msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(static_cast<double>(nums[msgInfo->fields[i].array_length - 1]));
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    double d = *(reinterpret_cast<double*>(m + msgInfo->fields[i].wire_offset));
                    f->updateValue(QString::number(d), static_cast<qreal>(d));
                }
                break;
            case MAVLINK_TYPE_UINT64_T:
                if (msgInfo->fields[i].array_length > 0) {
                    uint64_t* nums = reinterpret_cast<uint64_t*>(m + msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[msgInfo->fields[i].array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    uint64_t n = *(reinterpret_cast<uint64_t*>(m + msgInfo->fields[i].wire_offset));
                    //-- Special case
                    if(_message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME) {
                        QDateTime d = QDateTime::fromMSecsSinceEpoch(n/1000,Qt::UTC,0);
                        f->updateValue(d.toString("yyyy MM dd HH:mm:ss"), static_cast<qreal>(n));
                    } else {
                        f->updateValue(QString::number(n), static_cast<qreal>(n));
                    }
                }
                break;
            case MAVLINK_TYPE_INT64_T:
                if (msgInfo->fields[i].array_length > 0) {
                    int64_t* nums = reinterpret_cast<int64_t*>(m + msgInfo->fields[i].wire_offset);
                    // Enforce null termination
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < msgInfo->fields[i].array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[msgInfo->fields[i].array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    int64_t n = *(reinterpret_cast<int64_t*>(m + msgInfo->fields[i].wire_offset));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            }
        }
    }
    emit messageChanged();
}

//-----------------------------------------------------------------------------
QGCMAVLinkVehicle::QGCMAVLinkVehicle(QObject* parent, quint8 id)
    : QObject(parent)
    , _id(id)
{
    qCDebug(MAVLinkInspectorLog) << "New Vehicle:" << id;
}

//-----------------------------------------------------------------------------
QGCMAVLinkMessage*
QGCMAVLinkVehicle::findMessage(uint32_t id, uint8_t cid)
{
    for(int i = 0; i < _messages.count(); i++) {
        QGCMAVLinkMessage* m = qobject_cast<QGCMAVLinkMessage*>(_messages.get(i));
        if(m) {
            if(m->id() == id && m->cid() == cid) {
                return m;
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
static bool
messages_sort(QObject* a, QObject* b)
{
    QGCMAVLinkMessage* aa = qobject_cast<QGCMAVLinkMessage*>(a);
    QGCMAVLinkMessage* bb = qobject_cast<QGCMAVLinkMessage*>(b);
    if(!aa || !bb) return false;
    if(aa->id() == bb->id()) return aa->cid() < bb->cid();
    return aa->id() < bb->id();
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkVehicle::append(QGCMAVLinkMessage* message)
{
    _messages.append(message);
    //-- Sort messages by id and then cid
    if(_messages.count() > 0) {
        std::sort(_messages.objectList()->begin(), _messages.objectList()->end(), messages_sort);
        for(int i = 0; i < _messages.count(); i++) {
            QGCMAVLinkMessage* m = qobject_cast<QGCMAVLinkMessage*>(_messages.get(i));
            if(m) {
                emit m->indexChanged();
            }
        }
        _checkCompID(message);
    }
    emit messagesChanged();
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkVehicle::_checkCompID(QGCMAVLinkMessage* message)
{
    if(_compIDsStr.isEmpty()) {
        _compIDsStr << tr("All");
    }
    if(!_compIDs.contains(static_cast<int>(message->cid()))) {
        int cid = static_cast<int>(message->cid());
        _compIDs.append(cid);
        _compIDsStr << QString::number(cid);
        emit compIDsChanged();
    }
}

//-----------------------------------------------------------------------------
MAVLinkInspectorController::MAVLinkInspectorController()
{
    MultiVehicleManager* multiVehicleManager = qgcApp()->toolbox()->multiVehicleManager();
    connect(multiVehicleManager, &MultiVehicleManager::vehicleAdded,   this, &MAVLinkInspectorController::_vehicleAdded);
    connect(multiVehicleManager, &MultiVehicleManager::vehicleRemoved, this, &MAVLinkInspectorController::_vehicleRemoved);
    MAVLinkProtocol* mavlinkProtocol = qgcApp()->toolbox()->mavlinkProtocol();
    connect(mavlinkProtocol, &MAVLinkProtocol::messageReceived, this, &MAVLinkInspectorController::_receiveMessage);
    connect(&_updateTimer, &QTimer::timeout, this, &MAVLinkInspectorController::_refreshFrequency);
    _updateTimer.start(1000);
    MultiVehicleManager *manager = qgcApp()->toolbox()->multiVehicleManager();
    connect(manager, &MultiVehicleManager::activeVehicleChanged, this, &MAVLinkInspectorController::_setActiveVehicle);
    _rangeXMax = QDateTime::fromMSecsSinceEpoch(0);
    _rangeXMin = QDateTime::fromMSecsSinceEpoch(std::numeric_limits<qint64>::max());
    _timeScales << tr("5 Sec");
    _timeScales << tr("10 Sec");
    _timeScales << tr("30 Sec");
    _timeScales << tr("1 Min");
    _timeScales << tr("2 Min");
    _timeScales << tr("5 Min");
}

//-----------------------------------------------------------------------------
MAVLinkInspectorController::~MAVLinkInspectorController()
{
    _reset();
}


//----------------------------------------------------------------------------------------
void
MAVLinkInspectorController::_setActiveVehicle(Vehicle* vehicle)
{
    if(vehicle) {
        QGCMAVLinkVehicle* v = _findVehicle(static_cast<uint8_t>(vehicle->id()));
        if(v) {
            _activeVehicle = v;
        } else {
            _activeVehicle = nullptr;
        }
    } else {
        _activeVehicle = nullptr;
    }
    emit activeVehiclesChanged();
}

//-----------------------------------------------------------------------------
QGCMAVLinkVehicle*
MAVLinkInspectorController::_findVehicle(uint8_t id)
{
    for(int i = 0; i < _vehicles.count(); i++) {
        QGCMAVLinkVehicle* v = qobject_cast<QGCMAVLinkVehicle*>(_vehicles.get(i));
        if(v) {
            if(v->id() == id) {
                return v;
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_refreshFrequency()
{
    for(int i = 0; i < _vehicles.count(); i++) {
        QGCMAVLinkVehicle* v = qobject_cast<QGCMAVLinkVehicle*>(_vehicles.get(i));
        if(v) {
            for(int i = 0; i < v->messages()->count(); i++) {
                QGCMAVLinkMessage* m = qobject_cast<QGCMAVLinkMessage*>(v->messages()->get(i));
                if(m) {
                    m->updateFreq();
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_vehicleAdded(Vehicle* vehicle)
{
    QGCMAVLinkVehicle* v = _findVehicle(static_cast<uint8_t>(vehicle->id()));
    if(v) {
        v->messages()->clearAndDeleteContents();
        emit v->messagesChanged();
    } else {
        v = new QGCMAVLinkVehicle(this, static_cast<uint8_t>(vehicle->id()));
        _vehicles.append(v);
        _vehicleNames.append(tr("Vehicle %1").arg(vehicle->id()));
    }
    emit vehiclesChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_vehicleRemoved(Vehicle* vehicle)
{
    QGCMAVLinkVehicle* v = _findVehicle(static_cast<uint8_t>(vehicle->id()));
    if(v) {
        v->deleteLater();
        _vehicles.removeOne(v);
        QString vs = tr("Vehicle %1").arg(vehicle->id());
        _vehicleNames.removeOne(vs);
        emit vehiclesChanged();
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_receiveMessage(LinkInterface*, mavlink_message_t message)
{
    QGCMAVLinkMessage* m = nullptr;
    QGCMAVLinkVehicle* v = _findVehicle(message.sysid);
    if(!v) {
        v = new QGCMAVLinkVehicle(this, message.sysid);
        _vehicles.append(v);
        _vehicleNames.append(tr("Vehicle %1").arg(message.sysid));
        emit vehiclesChanged();
        if(!_activeVehicle) {
            _activeVehicle = v;
            emit activeVehiclesChanged();
        }
    } else {
        m = v->findMessage(message.msgid, message.compid);
    }
    if(!m) {
        m = new QGCMAVLinkMessage(this, &message);
        v->append(m);
    } else {
        m->update(&message);
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_reset()
{
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::addChartField(QGCMAVLinkMessageField* field)
{
    QVariant f = QVariant::fromValue(field);
    for(int i = 0; i < _chartFields.count(); i++) {
        if(_chartFields.at(i) == f) {
            return;
        }
    }
    _chartFields.append(f);
    emit chartFieldCountChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::delChartField(QGCMAVLinkMessageField* field)
{
    QVariant f = QVariant::fromValue(field);
    for(int i = 0; i < _chartFields.count(); i++) {
        if(_chartFields.at(i) == f) {
            _chartFields.removeAt(i);
            emit chartFieldCountChanged();
            if(_chartFields.count() == 0) {
                _rangeXMax = QDateTime::fromMSecsSinceEpoch(0);
                _rangeXMin = QDateTime::fromMSecsSinceEpoch(std::numeric_limits<qint64>::max());
            }
            return;
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::updateSeries(int index, QAbstractSeries* series)
{
    if(index < _chartFields.count() && series) {
        QGCMAVLinkMessageField* f = qvariant_cast<QGCMAVLinkMessageField*>(_chartFields.at(index));
        QLineSeries* lineSeries = static_cast<QLineSeries*>(series);
        lineSeries->replace(f->series());
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::setTimeScale(quint32 t)
{
    _timeScale = t;
    emit timeScaleChanged();
    updateXRange();
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::updateXRange()
{
    int ts = 5 * 1000;
    switch(_timeScale) {
        case 1: ts = 10 * 1000; break;
        case 2: ts = 30 * 1000; break;
        case 3: ts = 60 * 1000; break;
        case 4: ts = 2 * 60 * 1000; break;
        case 5: ts = 5 * 60 * 1000; break;
    }
    qint64 t = static_cast<qint64>(QGC::groundTimeMilliseconds());
    _rangeXMax = QDateTime::fromMSecsSinceEpoch(t);
    _rangeXMin = QDateTime::fromMSecsSinceEpoch(t - ts);
    emit rangeMinXChanged();
    emit rangeMaxXChanged();
}
