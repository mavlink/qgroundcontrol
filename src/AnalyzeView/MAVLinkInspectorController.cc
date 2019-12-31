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
void
QGCMAVLinkMessageField::addSeries(QAbstractSeries* series, bool left)
{
    if(!_pSeries) {
        _left = left;
        _pSeries = series;
        emit seriesChanged();
        _dataIndex = 0;
        _msg->msgCtl()->addChartField(this, left);
        _msg->select();
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessageField::delSeries()
{
    if(_pSeries) {
        _values.clear();
        _msg->msgCtl()->delChartField(this, _left);
        QLineSeries* lineSeries = static_cast<QLineSeries*>(_pSeries);
        lineSeries->replace(_values);
        _pSeries = nullptr;
        _left = false;
        emit seriesChanged();
        _msg->select();
    }
}

//-----------------------------------------------------------------------------
QString
QGCMAVLinkMessageField::label()
{
    //-- Label is message name + field name
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
QGCMAVLinkMessageField::updateValue(QString newValue, qreal v)
{
    if(_value != newValue) {
        _value = newValue;
        emit valueChanged();
    }
    if(_pSeries) {
        int count = _values.count();
        //-- Arbitrary limit of 1 minute of data at 50Hz for now
        if(count < (50 * 60)) {
            QPointF p(QGC::groundTimeMilliseconds(), v);
            _values.append(p);
        } else {
            if(_dataIndex >= count) _dataIndex = 0;
            _values[_dataIndex].setX(QGC::groundTimeMilliseconds());
            _values[_dataIndex].setY(v);
            _dataIndex++;
        }
        //-- Auto Range
        if((!_left && _msg->msgCtl()->rightRangeIdx() == 0) || (_left && _msg->msgCtl()->leftRangeIdx() == 0)) {
            qreal vmin  = std::numeric_limits<qreal>::max();
            qreal vmax  = std::numeric_limits<qreal>::min();
            for(int i = 0; i < _values.count(); i++) {
                qreal v = _values[i].y();
                if(vmax < v) vmax = v;
                if(vmin > v) vmin = v;
            }
            bool changed = false;
            if(std::abs(_rangeMin - vmin) > 0.000001) {
                _rangeMin = vmin;
                changed = true;
            }
            if(std::abs(_rangeMax - vmax) > 0.000001) {
                _rangeMax = vmax;
                changed = true;
            }
            if(changed) {
                _msg->msgCtl()->updateYRange(_left);
            }
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
        QLineSeries* lineSeries = static_cast<QLineSeries*>(_pSeries);
        lineSeries->replace(_values);
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
MAVLinkInspectorController::TimeScale_st::TimeScale_st(QObject* parent, const QString& l, uint32_t t)
    : QObject(parent)
    , label(l)
    , timeScale(t)
{
}

//-----------------------------------------------------------------------------
MAVLinkInspectorController::Range_st::Range_st(QObject* parent, const QString& l, qreal r)
    : QObject(parent)
    , label(l)
    , range(r)
{
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
    _timeScaleSt.append(new TimeScale_st(this, tr("5 Sec"),   5 * 1000));
    _timeScaleSt.append(new TimeScale_st(this, tr("10 Sec"), 10 * 1000));
    _timeScaleSt.append(new TimeScale_st(this, tr("30 Sec"), 30 * 1000));
    _timeScaleSt.append(new TimeScale_st(this, tr("60 Sec"), 60 * 1000));
    emit timeScalesChanged();
    _rangeSt.append(new Range_st(this, tr("Auto"),    0));
    _rangeSt.append(new Range_st(this, tr("10,000"),  10000));
    _rangeSt.append(new Range_st(this, tr("1,000"),   1000));
    _rangeSt.append(new Range_st(this, tr("100"),     100));
    _rangeSt.append(new Range_st(this, tr("10"),      10));
    _rangeSt.append(new Range_st(this, tr("1"),       1));
    _rangeSt.append(new Range_st(this, tr("0.1"),     0.1));
    _rangeSt.append(new Range_st(this, tr("0.01"),    0.01));
    _rangeSt.append(new Range_st(this, tr("0.001"),   0.001));
    _rangeSt.append(new Range_st(this, tr("0.0001"),  0.0001));
    emit rangeListChanged();
    updateXRange();
}

//-----------------------------------------------------------------------------
MAVLinkInspectorController::~MAVLinkInspectorController()
{
}

//----------------------------------------------------------------------------------------
QStringList
MAVLinkInspectorController::timeScales()
{
    if(!_timeScales.count()) {
        for(int i = 0; i < _timeScaleSt.count(); i++) {
            _timeScales << _timeScaleSt[i]->label;
        }
    }
    return _timeScales;
}

//----------------------------------------------------------------------------------------
QStringList
MAVLinkInspectorController::rangeList()
{
    if(!_rangeList.count()) {
        for(int i = 0; i < _rangeSt.count(); i++) {
            _rangeList << _rangeSt[i]->label;
        }
    }
    return _rangeList;
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::setLeftRangeIdx(quint32 r)
{
    if(r < static_cast<quint32>(_rangeSt.count())) {
        _leftRangeIndex = r;
        _timeRange = _rangeSt[static_cast<int>(r)]->range;
        emit leftRangeChanged();
        //-- If not Auto, use defined range
        if(_leftRangeIndex > 0) {
            _leftRangeMin = -_timeRange;
            emit leftRangeMinChanged();
            _leftRangeMax = _timeRange;
            emit leftRangeMaxChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::setRightRangeIdx(quint32 r)
{
    if(r < static_cast<quint32>(_rangeSt.count())) {
        _rightRangeIndex = r;
        _timeRange = _rangeSt[static_cast<int>(r)]->range;
        emit rightRangeChanged();
        //-- If not Auto, use defined range
        if(_rightRangeIndex > 0) {
            _rightRangeMin = -_timeRange;
            emit rightRangeMinChanged();
            _rightRangeMax = _timeRange;
            emit rightRangeMaxChanged();
        }
    }
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
MAVLinkInspectorController::addChartField(QGCMAVLinkMessageField* field, bool left)
{
    QVariant f = QVariant::fromValue(field);
    QVariantList* pList;
    if(left) {
        pList = &_leftChartFields;
    } else {
        pList = &_rightChartFields;
    }
    for(int i = 0; i < pList->count(); i++) {
        if(pList->at(i) == f) {
            return;
        }
    }
    pList->append(f);
    if(left) {
        emit leftChartFieldsChanged();
    } else {
        emit rightChartFieldsChanged();
    }
    emit seriesCountChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::delChartField(QGCMAVLinkMessageField* field, bool left)
{
    QVariant f = QVariant::fromValue(field);
    QVariantList* pList;
    if(left) {
        pList = &_leftChartFields;
    } else {
        pList = &_rightChartFields;
    }
    for(int i = 0; i < pList->count(); i++) {
        if(pList->at(i) == f) {
            pList->removeAt(i);
            if(left) {
                emit leftChartFieldsChanged();
            } else {
                emit rightChartFieldsChanged();
            }
            emit seriesCountChanged();
            return;
        }
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
    if(_timeScale < static_cast<quint32>(_timeScaleSt.count())) {
        qint64 t = static_cast<qint64>(QGC::groundTimeMilliseconds());
        _rangeXMax = QDateTime::fromMSecsSinceEpoch(t);
        _rangeXMin = QDateTime::fromMSecsSinceEpoch(t - _timeScaleSt[static_cast<int>(_timeScale)]->timeScale);
        emit rangeMinXChanged();
        emit rangeMaxXChanged();
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::updateYRange(bool left)
{
    QVariantList* pList;
    if(left) {
        pList = &_leftChartFields;
    } else {
        pList = &_rightChartFields;
    }
    if(pList->count()) {
        qreal vmin  = std::numeric_limits<qreal>::max();
        qreal vmax  = std::numeric_limits<qreal>::min();
        for(int i = 0; i < pList->count(); i++) {
            QObject* object = qvariant_cast<QObject*>(pList->at(i));
            QGCMAVLinkMessageField* pField = qobject_cast<QGCMAVLinkMessageField*>(object);
            if(pField) {
                if(vmax < pField->rangeMax()) vmax = pField->rangeMax();
                if(vmin > pField->rangeMin()) vmin = pField->rangeMin();
            }
        }
        if(left) {
            if(std::abs(_leftRangeMin - vmin) > 0.000001) {
                _leftRangeMin = vmin;
                emit leftRangeMinChanged();
            }
            if(std::abs(_leftRangeMax - vmax) > 0.000001) {
                _leftRangeMax = vmax;
                emit leftRangeMaxChanged();
            }
        } else {
            if(std::abs(_rightRangeMin - vmin) > 0.000001) {
                _rightRangeMin = vmin;
                emit rightRangeMinChanged();
            }
            if(std::abs(_rightRangeMax - vmax) > 0.000001) {
                _rightRangeMax = vmax;
                emit rightRangeMaxChanged();
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::addSeries(QGCMAVLinkMessageField* field, QAbstractSeries* series, bool left)
{
    if(field) {
        field->addSeries(series, left);
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::delSeries(QGCMAVLinkMessageField* field)
{
    if(field) {
        field->delSeries();
    }
    if(_leftChartFields.count() == 0) {
        _leftRangeMin  = 0;
        _leftRangeMax  = 1;
        emit leftRangeMinChanged();
        emit leftRangeMaxChanged();
    }
    if(_rightChartFields.count() == 0) {
        _rightRangeMin = 0;
        _rightRangeMax = 1;
        emit rightRangeMinChanged();
        emit rightRangeMaxChanged();
    }
    if(_leftChartFields.count() == 0 && _rightChartFields.count() == 0) {
        updateXRange();
    }
}
