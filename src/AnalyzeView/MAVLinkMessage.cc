/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MAVLinkMessage.h"
#include "MAVLinkMessageField.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

#include <QtCore/QDateTime>

QGC_LOGGING_CATEGORY(MAVLinkMessageLog, "qgc.analyzeview.mavlinkmessage")

QGCMAVLinkMessage::QGCMAVLinkMessage(const mavlink_message_t &message, QObject *parent)
    : QObject(parent)
    , _message(message)
    , _fields(new QmlObjectListModel(this))
{
    // qCDebug(MAVLinkMessageLog) << Q_FUNC_INFO << this;

    const mavlink_message_info_t *msgInfo = mavlink_get_message_info(&_message);
    if (!msgInfo) {
        qCWarning(MAVLinkMessageLog) << "QGCMAVLinkMessage NULL msgInfo msgid" << _message.msgid;
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

        QGCMAVLinkMessageField *const field = new QGCMAVLinkMessageField(this, msgInfo->fields[i].name, type);
        (void) _fields->append(field);
    }
}

QGCMAVLinkMessage::~QGCMAVLinkMessage()
{
    _fields->clearAndDeleteContents();

    // qCDebug(MAVLinkMessageLog) << Q_FUNC_INFO << this;
}

void QGCMAVLinkMessage::updateFieldSelection()
{
    bool sel = false;

    for (int i = 0; i < _fields->count(); ++i) {
        const QGCMAVLinkMessageField *const field = qobject_cast<const QGCMAVLinkMessageField*>(_fields->get(i));
        if(field && field->selected()) {
            sel = true;
            break;
        }
    }

    if (sel != _fieldSelected) {
        _fieldSelected = sel;
        emit fieldSelectedChanged();
    }
}

void QGCMAVLinkMessage::updateFreq()
{
    const quint64 msgCount = _count - _lastCount;
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
    if (rate != _targetRateHz) {
        _targetRateHz = rate;
        emit targetRateHzChanged();
    }
}

void QGCMAVLinkMessage::update(const mavlink_message_t &message)
{
    _count++;
    _message = message;

    if (_selected) {
        // Don't update field info unless selected to reduce perf hit of message processing
        _updateFields();
    }

    emit countChanged();
}

void QGCMAVLinkMessage::_updateFields()
{
    const mavlink_message_info_t *const msgInfo = mavlink_get_message_info(&_message);
    if (!msgInfo) {
        qCWarning(MAVLinkMessageLog) << "QGCMAVLinkMessage::update NULL msgInfo msgid" << _message.msgid;
        return;
    }

    if (_fields->count() != static_cast<int>(msgInfo->num_fields)) {
        qCWarning(MAVLinkMessageLog) << "QGCMAVLinkMessage::update msgInfo field count mismatch msgid" << _message.msgid;
        return;
    }

    uint8_t *const msg = reinterpret_cast<uint8_t*>(&_message.payload64[0]);
    for (unsigned int i = 0; i < msgInfo->num_fields; ++i) {
        QGCMAVLinkMessageField *const field = qobject_cast<QGCMAVLinkMessageField*>(_fields->get(static_cast<int>(i)));
        if (!field) {
            continue;
        }

        const unsigned int offset = msgInfo->fields[i].wire_offset;
        const unsigned int array_length = msgInfo->fields[i].array_length;
        static const unsigned int array_buffer_length = (MAVLINK_MAX_PAYLOAD_LEN + MAVLINK_NUM_CHECKSUM_BYTES + 7);

        switch (msgInfo->fields[i].type) {
        case MAVLINK_TYPE_CHAR:
            field->setSelectable(false);
            if (array_length > 0) {
                char *const str = reinterpret_cast<char*>(msg + offset);
                str[array_length - 1] = '\0';
                QString value(str);
                field->updateValue(value, 0);
            } else {
                char num = *(reinterpret_cast<char*>(msg + offset));
                QString value(num);
                field->updateValue(value, 0);
            }
            break;
        case MAVLINK_TYPE_UINT8_T:
            if (array_length > 0) {
                uint8_t *const nums = msg + offset;
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                    string += tmp.arg(nums[j]);
                }
                string += QString::number(nums[array_length - 1]);
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                const uint8_t num = *(msg + offset);
                field->updateValue(QString::number(num), static_cast<qreal>(num));
            }
            break;
        case MAVLINK_TYPE_INT8_T:
            if (array_length > 0) {
                int8_t *const nums = reinterpret_cast<int8_t*>(msg + offset);
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                    string += tmp.arg(nums[j]);
                }
                string += QString::number(nums[array_length - 1]);
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                int8_t num = *(reinterpret_cast<int8_t*>(msg + offset));
                field->updateValue(QString::number(num), static_cast<qreal>(num));
            }
            break;
        case MAVLINK_TYPE_UINT16_T:
            if (array_length > 0) {
                uint16_t nums[array_buffer_length / sizeof(uint16_t)];
                (void) memcpy(nums, msg + offset,  sizeof(uint16_t) * array_length);
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                    string += tmp.arg(nums[j]);
                }
                string += QString::number(nums[array_length - 1]);
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                uint16_t num;
                (void) memcpy(&num, msg + offset, sizeof(uint16_t));
                field->updateValue(QString::number(num), static_cast<qreal>(num));
            }
            break;
        case MAVLINK_TYPE_INT16_T:
            if (array_length > 0) {
                int16_t nums[array_buffer_length / sizeof(int16_t)];
                (void) memcpy(nums, msg + offset,  sizeof(int16_t) * array_length);
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                    string += tmp.arg(nums[j]);
                }
                string += QString::number(nums[array_length - 1]);
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                int16_t num;
                (void) memcpy(&num, msg + offset, sizeof(int16_t));
                field->updateValue(QString::number(num), static_cast<qreal>(num));
            }
            break;
        case MAVLINK_TYPE_UINT32_T:
            if (array_length > 0) {
                uint32_t nums[array_buffer_length / sizeof(uint32_t)];
                (void) memcpy(nums, msg + offset,  sizeof(uint32_t) * array_length);
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                    string += tmp.arg(nums[j]);
                }
                string += QString::number(nums[array_length - 1]);
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                uint32_t num;
                (void) memcpy(&num, msg + offset, sizeof(uint32_t));
                if (_message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME) {
                    const QDateTime date = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(num),Qt::UTC,0);
                    field->updateValue(date.toString("HH:mm:ss"), static_cast<qreal>(num));
                } else {
                    field->updateValue(QString::number(num), static_cast<qreal>(num));
                }
            }
            break;
        case MAVLINK_TYPE_INT32_T:
            if (array_length > 0) {
                int32_t nums[array_buffer_length / sizeof(int32_t)];
                (void) memcpy(nums, msg + offset,  sizeof(int32_t) * array_length);
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                    string += tmp.arg(nums[j]);
                }
                string += QString::number(nums[array_length - 1]);
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                int32_t num;
                (void) memcpy(&num, msg + offset, sizeof(int32_t));
                field->updateValue(QString::number(num), static_cast<qreal>(num));
            }
            break;
        case MAVLINK_TYPE_FLOAT:
            if (array_length > 0) {
                float nums[array_buffer_length / sizeof(float)];
                (void) memcpy(nums, msg + offset,  sizeof(float) * array_length);
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                   string += tmp.arg(static_cast<double>(nums[j]));
                }
                string += QString::number(static_cast<double>(nums[array_length - 1]));
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                float num;
                (void) memcpy(&num, msg + offset, sizeof(float));
                field->updateValue(QString::number(static_cast<double>(num)), static_cast<qreal>(num));
            }
            break;
        case MAVLINK_TYPE_DOUBLE:
            if (array_length > 0) {
                double nums[array_buffer_length / sizeof(double)];
                (void) memcpy(nums, msg + offset,  sizeof(double) * array_length);
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                    string += tmp.arg(nums[j]);
                }
                string += QString::number(static_cast<double>(nums[array_length - 1]));
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                double num;
                (void) memcpy(&num, msg + offset, sizeof(double));
                field->updateValue(QString::number(num), static_cast<qreal>(num));
            }
            break;
        case MAVLINK_TYPE_UINT64_T:
            if (array_length > 0) {
                uint64_t nums[array_buffer_length / sizeof(uint64_t)];
                (void) memcpy(nums, msg + offset,  sizeof(uint64_t) * array_length);
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                    string += tmp.arg(nums[j]);
                }
                string += QString::number(nums[array_length - 1]);
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                uint64_t num;
                (void) memcpy(&num, msg + offset, sizeof(uint64_t));
                if (_message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME) {
                    const QDateTime date = QDateTime::fromMSecsSinceEpoch(num / 1000, Qt::UTC, 0);
                    field->updateValue(date.toString("yyyy MM dd HH:mm:ss"), static_cast<qreal>(num));
                } else {
                    field->updateValue(QString::number(num), static_cast<qreal>(num));
                }
            }
            break;
        case MAVLINK_TYPE_INT64_T:
            if (array_length > 0) {
                int64_t nums[array_buffer_length / sizeof(int64_t)];
                (void) memcpy(nums, msg + offset,  sizeof(int64_t) * array_length);
                QString tmp("%1, ");
                QString string;
                for (unsigned int j = 0; j < array_length - 1; ++j) {
                    string += tmp.arg(nums[j]);
                }
                string += QString::number(nums[array_length - 1]);
                field->updateValue(string, static_cast<qreal>(nums[0]));
            } else {
                int64_t num;
                (void) memcpy(&num, msg + offset, sizeof(int64_t));
                field->updateValue(QString::number(num), static_cast<qreal>(num));
            }
            break;
        default:
            continue;
        }
    }
}
