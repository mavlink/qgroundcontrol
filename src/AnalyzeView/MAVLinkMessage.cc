/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkMessage.h"
#include "MAVLinkMessageField.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MAVLinkMessageLog, "qgc.analyzeview.mavlinkmessage")

//-----------------------------------------------------------------------------
QGCMAVLinkMessage::QGCMAVLinkMessage(QObject *parent, mavlink_message_t* message)
    : QObject(parent)
{
    _message = *message;
    const mavlink_message_info_t* msgInfo = mavlink_get_message_info(message);
    if (!msgInfo) {
        qCWarning(MAVLinkMessageLog) << QStringLiteral("QGCMAVLinkMessage NULL msgInfo msgid(%1)").arg(message->msgid);
        return;
    }
    _name = QString(msgInfo->name);
    qCDebug(MAVLinkMessageLog) << "New Message:" << _name;
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
    _actualRateHz = (0.2 * _actualRateHz) + (0.8 * msgCount);
    _lastCount = _count;
    emit actualRateHzChanged();
}

void QGCMAVLinkMessage::setSelected(bool sel)
{
    if (_selected != sel) {
        _selected = sel;
        _updateFields();
        emit selectedChanged();
    }
}

void QGCMAVLinkMessage::setTargetRateHz(int32_t rate)
{
    if(rate != _targetRateHz)
    {
        _targetRateHz = rate;
        emit targetRateHzChanged();
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
