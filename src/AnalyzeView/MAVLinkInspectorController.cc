/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

#define UPDATE_FREQUENCY (1000 / 15)    // 15Hz

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
QGCMAVLinkMessageField::addSeries(MAVLinkChartController* chart, QAbstractSeries* series)
{
    if(!_pSeries) {
        _chart = chart;
        _pSeries = series;
        emit seriesChanged();
        _dataIndex = 0;
        _msg->updateFieldSelection();
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessageField::delSeries()
{
    if(_pSeries) {
        _values.clear();
        QLineSeries* lineSeries = static_cast<QLineSeries*>(_pSeries);
        lineSeries->replace(_values);
        _pSeries = nullptr;
        _chart   = nullptr;
        emit seriesChanged();
        _msg->updateFieldSelection();
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
int
QGCMAVLinkMessageField::chartIndex()
{
    if(_chart)
        return _chart->chartIndex();
    return 0;
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessageField::updateValue(QString newValue, qreal v)
{
    if(_value != newValue) {
        _value = newValue;
        emit valueChanged();
    }
    if(_pSeries && _chart) {
        int count = _values.count();
        //-- Arbitrary limit of 1 minute of data at 50Hz for now
        if(count < (50 * 60)) {
            QPointF p(QGC::bootTimeMilliseconds(), v);
            _values.append(p);
        } else {
            if(_dataIndex >= count) _dataIndex = 0;
            _values[_dataIndex].setX(QGC::bootTimeMilliseconds());
            _values[_dataIndex].setY(v);
            _dataIndex++;
        }
        //-- Auto Range
        if(_chart->rangeYIndex() == 0) {
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
                _chart->updateYRange();
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessageField::updateSeries()
{
    int count = _values.count();
    if (count > 1) {
        QList<QPointF> s;
        s.reserve(count);
        int idx = _dataIndex;
        for(int i = 0; i < count; i++, idx++) {
            if(idx >= count) idx = 0;
            QPointF p(_values[idx]);
            s.append(p);
        }
        QLineSeries* lineSeries = static_cast<QLineSeries*>(_pSeries);
        lineSeries->replace(s);
    }
}

//-----------------------------------------------------------------------------
QGCMAVLinkMessage::QGCMAVLinkMessage(QObject *parent, mavlink_message_t* message)
    : QObject(parent)
{
    _message = *message;
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info(message);
    if (!msgInfo) {
        qCWarning(MAVLinkInspectorLog) << QStringLiteral("QGCMAVLinkMessage NULL msgInfo msgid(%1)").arg(message->msgid);
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
QGCMAVLinkMessage::~QGCMAVLinkMessage()
{
    _fields.clearAndDeleteContents();
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessage::updateFieldSelection()
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
    if(sel != _fieldSelected) {
        _fieldSelected = sel;
        emit fieldSelectedChanged();
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

void QGCMAVLinkMessage::setSelected(bool sel)
{
    if (_selected != sel) {
        _selected = sel;
        _updateFields();
        emit selectedChanged();
    }
}

//-----------------------------------------------------------------------------
void
QGCMAVLinkMessage::update(mavlink_message_t* message)
{
    _count++;
    _message = *message;

    if (_selected) {
        // Don't update field info unless selected to reduce perf hit of message processing
        _updateFields();
    }
    emit countChanged();
}

void QGCMAVLinkMessage::_updateFields(void)
{
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info(&_message);
    if (!msgInfo) {
        qWarning() << QStringLiteral("QGCMAVLinkMessage::update NULL msgInfo msgid(%1)").arg(_message.msgid);
        return;
    }
    if(_fields.count() != static_cast<int>(msgInfo->num_fields)) {
        qWarning() << QStringLiteral("QGCMAVLinkMessage::update msgInfo field count mismatch msgid(%1)").arg(_message.msgid);
        return;
    }
    uint8_t* m = reinterpret_cast<uint8_t*>(&_message.payload64[0]);
    for (unsigned int i = 0; i < msgInfo->num_fields; ++i) {
        QGCMAVLinkMessageField* f = qobject_cast<QGCMAVLinkMessageField*>(_fields.get(static_cast<int>(i)));
        if(f) {
            const unsigned int offset = msgInfo->fields[i].wire_offset;
            const unsigned int array_length = msgInfo->fields[i].array_length;
            static const unsigned int array_buffer_length = (MAVLINK_MAX_PAYLOAD_LEN + MAVLINK_NUM_CHECKSUM_BYTES + 7);
            switch (msgInfo->fields[i].type) {
            case MAVLINK_TYPE_CHAR:
                f->setSelectable(false);
                if (array_length > 0) {
                    char* str = reinterpret_cast<char*>(m + offset);
                    // Enforce null termination
                    str[array_length - 1] = '\0';
                    QString v(str);
                    f->updateValue(v, 0);
                } else {
                    // Single char
                    char b = *(reinterpret_cast<char*>(m + offset));
                    QString v(b);
                    f->updateValue(v, 0);
                }
                break;
            case MAVLINK_TYPE_UINT8_T:
                if (array_length > 0) {
                    uint8_t* nums = m + offset;
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    uint8_t u = *(m + offset);
                    f->updateValue(QString::number(u), static_cast<qreal>(u));
                }
                break;
            case MAVLINK_TYPE_INT8_T:
                if (array_length > 0) {
                    int8_t* nums = reinterpret_cast<int8_t*>(m + offset);
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    int8_t n = *(reinterpret_cast<int8_t*>(m + offset));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            case MAVLINK_TYPE_UINT16_T:
                if (array_length > 0) {
                    uint16_t nums[array_buffer_length / sizeof(uint16_t)];
                    memcpy(nums, m + offset,  sizeof(uint16_t) * array_length);
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    uint16_t n;
                    memcpy(&n, m + offset, sizeof(uint16_t));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            case MAVLINK_TYPE_INT16_T:
                if (array_length > 0) {
                    int16_t nums[array_buffer_length / sizeof(int16_t)];
                    memcpy(nums, m + offset,  sizeof(int16_t) * array_length);
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    int16_t n;
                    memcpy(&n, m + offset, sizeof(int16_t));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            case MAVLINK_TYPE_UINT32_T:
                if (array_length > 0) {
                    uint32_t nums[array_buffer_length / sizeof(uint32_t)];
                    memcpy(nums, m + offset,  sizeof(uint32_t) * array_length);
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    uint32_t n;
                    memcpy(&n, m + offset, sizeof(uint32_t));
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
                if (array_length > 0) {
                    int32_t nums[array_buffer_length / sizeof(int32_t)];
                    memcpy(nums, m + offset,  sizeof(int32_t) * array_length);
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    int32_t n;
                    memcpy(&n, m + offset, sizeof(int32_t));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            case MAVLINK_TYPE_FLOAT:
                if (array_length > 0) {
                    float nums[array_buffer_length / sizeof(float)];
                    memcpy(nums, m + offset,  sizeof(float) * array_length);
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                       string += tmp.arg(static_cast<double>(nums[j]));
                    }
                    string += QString::number(static_cast<double>(nums[array_length - 1]));
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    float fv;
                    memcpy(&fv, m + offset, sizeof(float));
                    f->updateValue(QString::number(static_cast<double>(fv)), static_cast<qreal>(fv));
                }
                break;
            case MAVLINK_TYPE_DOUBLE:
                if (array_length > 0) {
                    double nums[array_buffer_length / sizeof(double)];
                    memcpy(nums, m + offset,  sizeof(double) * array_length);
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(static_cast<double>(nums[array_length - 1]));
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    double d;
                    memcpy(&d, m + offset, sizeof(double));
                    f->updateValue(QString::number(d), static_cast<qreal>(d));
                }
                break;
            case MAVLINK_TYPE_UINT64_T:
                if (array_length > 0) {
                    uint64_t nums[array_buffer_length / sizeof(uint64_t)];
                    memcpy(nums, m + offset,  sizeof(uint64_t) * array_length);
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    uint64_t n;
                    memcpy(&n, m + offset, sizeof(uint64_t));
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
                if (array_length > 0) {
                    int64_t nums[array_buffer_length / sizeof(int64_t)];
                    memcpy(nums, m + offset,  sizeof(int64_t) * array_length);
                    QString tmp("%1, ");
                    QString string;
                    for (unsigned int j = 0; j < array_length - 1; ++j) {
                        string += tmp.arg(nums[j]);
                    }
                    string += QString::number(nums[array_length - 1]);
                    f->updateValue(string, static_cast<qreal>(nums[0]));
                } else {
                    // Single value
                    int64_t n;
                    memcpy(&n, m + offset, sizeof(int64_t));
                    f->updateValue(QString::number(n), static_cast<qreal>(n));
                }
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
QGCMAVLinkSystem::QGCMAVLinkSystem(QObject* parent, quint8 id)
    : QObject(parent)
    , _id(id)
{
    qCDebug(MAVLinkInspectorLog) << "New Vehicle:" << id;
}

//-----------------------------------------------------------------------------
QGCMAVLinkSystem::~QGCMAVLinkSystem()
{
    _messages.clearAndDeleteContents();
}

//-----------------------------------------------------------------------------
QGCMAVLinkMessage*
QGCMAVLinkSystem::findMessage(uint32_t id, uint8_t cid)
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
    //-- Sort messages by id and then cid
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
    if(!_compIDs.contains(static_cast<int>(message->cid()))) {
        int cid = static_cast<int>(message->cid());
        _compIDs.append(cid);
        _compIDsStr << tr("Comp %1").arg(cid);
        emit compIDsChanged();
    }
}

//-----------------------------------------------------------------------------
MAVLinkChartController::MAVLinkChartController(MAVLinkInspectorController *parent, int index)
    : QObject(parent)
    , _index(index)
    , _controller(parent)
{
    connect(&_updateSeriesTimer, &QTimer::timeout, this, &MAVLinkChartController::_refreshSeries);
    updateXRange();
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::setRangeYIndex(quint32 r)
{
    if(r < static_cast<quint32>(_controller->rangeSt().count())) {
        _rangeYIndex = r;
        qreal range = _controller->rangeSt()[static_cast<int>(r)]->range;
        emit rangeYIndexChanged();
        //-- If not Auto, use defined range
        if(_rangeYIndex > 0) {
            _rangeYMin = -range;
            emit rangeYMinChanged();
            _rangeYMax = range;
            emit rangeYMaxChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::setRangeXIndex(quint32 t)
{
    _rangeXIndex = t;
    emit rangeXIndexChanged();
    updateXRange();
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::updateXRange()
{
    if(_rangeXIndex < static_cast<quint32>(_controller->timeScaleSt().count())) {
        qint64 t = static_cast<qint64>(QGC::bootTimeMilliseconds());
        _rangeXMax = QDateTime::fromMSecsSinceEpoch(t);
        _rangeXMin = QDateTime::fromMSecsSinceEpoch(t - _controller->timeScaleSt()[static_cast<int>(_rangeXIndex)]->timeScale);
        emit rangeXMinChanged();
        emit rangeXMaxChanged();
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::updateYRange()
{
    if(_chartFields.count()) {
        qreal vmin  = std::numeric_limits<qreal>::max();
        qreal vmax  = std::numeric_limits<qreal>::min();
        for(int i = 0; i < _chartFields.count(); i++) {
            QObject* object = qvariant_cast<QObject*>(_chartFields.at(i));
            QGCMAVLinkMessageField* pField = qobject_cast<QGCMAVLinkMessageField*>(object);
            if(pField) {
                if(vmax < pField->rangeMax()) vmax = pField->rangeMax();
                if(vmin > pField->rangeMin()) vmin = pField->rangeMin();
            }
        }
        if(std::abs(_rangeYMin - vmin) > 0.000001) {
            _rangeYMin = vmin;
            emit rangeYMinChanged();
        }
        if(std::abs(_rangeYMax - vmax) > 0.000001) {
            _rangeYMax = vmax;
            emit rangeYMaxChanged();
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::_refreshSeries()
{
    updateXRange();
    for(int i = 0; i < _chartFields.count(); i++) {
        QObject* object = qvariant_cast<QObject*>(_chartFields.at(i));
        QGCMAVLinkMessageField* pField = qobject_cast<QGCMAVLinkMessageField*>(object);
        if(pField) {
            pField->updateSeries();
        }
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::addSeries(QGCMAVLinkMessageField* field, QAbstractSeries* series)
{
    if(field && series) {
        QVariant f = QVariant::fromValue(field);
        for(int i = 0; i < _chartFields.count(); i++) {
            if(_chartFields.at(i) == f) {
                return;
            }
        }
        _chartFields.append(f);
        field->addSeries(this, series);
        emit chartFieldsChanged();
        _updateSeriesTimer.start(UPDATE_FREQUENCY);
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkChartController::delSeries(QGCMAVLinkMessageField* field)
{
    if(field) {
        field->delSeries();
        QVariant f = QVariant::fromValue(field);
        for(int i = 0; i < _chartFields.count(); i++) {
            if(_chartFields.at(i) == f) {
                _chartFields.removeAt(i);
                emit chartFieldsChanged();
                if(_chartFields.count() == 0) {
                    updateXRange();
                    _updateSeriesTimer.stop();
                }
                return;
            }
        }
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
    connect(&_updateFrequencyTimer, &QTimer::timeout, this, &MAVLinkInspectorController::_refreshFrequency);
    _updateFrequencyTimer.start(1000);
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
}

//-----------------------------------------------------------------------------
MAVLinkInspectorController::~MAVLinkInspectorController()
{
    _charts.clearAndDeleteContents();
    _systems.clearAndDeleteContents();
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

//----------------------------------------------------------------------------------------
void
MAVLinkInspectorController::_setActiveVehicle(Vehicle* vehicle)
{
    if(vehicle) {
        QGCMAVLinkSystem* v = _findVehicle(static_cast<uint8_t>(vehicle->id()));
        if(v) {
            _activeSystem = v;
        } else {
            _activeSystem = nullptr;
        }
    } else {
        _activeSystem = nullptr;
    }
    emit activeSystemChanged();
}

//-----------------------------------------------------------------------------
QGCMAVLinkSystem*
MAVLinkInspectorController::_findVehicle(uint8_t id)
{
    for(int i = 0; i < _systems.count(); i++) {
        QGCMAVLinkSystem* v = qobject_cast<QGCMAVLinkSystem*>(_systems.get(i));
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
    for(int i = 0; i < _systems.count(); i++) {
        QGCMAVLinkSystem* v = qobject_cast<QGCMAVLinkSystem*>(_systems.get(i));
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
    QGCMAVLinkSystem* v = _findVehicle(static_cast<uint8_t>(vehicle->id()));
    if(v) {
        v->messages()->clearAndDeleteContents();
    } else {
        v = new QGCMAVLinkSystem(this, static_cast<uint8_t>(vehicle->id()));
        _systems.append(v);
        _systemNames.append(tr("System %1").arg(vehicle->id()));
    }
    emit systemsChanged();
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_vehicleRemoved(Vehicle* vehicle)
{
    QGCMAVLinkSystem* v = _findVehicle(static_cast<uint8_t>(vehicle->id()));
    if(v) {
        v->deleteLater();
        _systems.removeOne(v);
        QString vs = tr("System %1").arg(vehicle->id());
        _systemNames.removeOne(vs);
        emit systemsChanged();
    }
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::_receiveMessage(LinkInterface*, mavlink_message_t message)
{
    QGCMAVLinkMessage* m = nullptr;
    QGCMAVLinkSystem* v = _findVehicle(message.sysid);
    if(!v) {
        v = new QGCMAVLinkSystem(this, message.sysid);
        _systems.append(v);
        _systemNames.append(tr("System %1").arg(message.sysid));
        emit systemsChanged();
        if(!_activeSystem) {
            _activeSystem = v;
            emit activeSystemChanged();
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
MAVLinkChartController*
MAVLinkInspectorController::createChart()
{
    MAVLinkChartController* pChart = new MAVLinkChartController(this, _charts.count());
    QQmlEngine::setObjectOwnership(pChart, QQmlEngine::CppOwnership);
    _charts.append(pChart);
    emit chartsChanged();
    return pChart;
}

//-----------------------------------------------------------------------------
void
MAVLinkInspectorController::deleteChart(MAVLinkChartController* chart)
{
    if(chart) {
        for(int i = 0; i < _charts.count(); i++) {
            MAVLinkChartController* c = qobject_cast<MAVLinkChartController*>(_charts.get(i));
            if(c && c == chart) {
                _charts.removeOne(c);
                delete c;
                break;
            }
        }
        emit chartsChanged();
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

void MAVLinkInspectorController::setActiveSystem(int systemId)
{
    QGCMAVLinkSystem* v = _findVehicle(systemId);
    if (v != _activeSystem) {
        _activeSystem = v;
        emit activeSystemChanged();
    }
}

