#include "MAVLinkMessage.h"
#include "MAVLinkInstanceFields.h"
#include "MAVLinkLib.h"
#include "MAVLinkMessageField.h"
#include "QGCLoggingCategory.h"
#include "QmlObjectListModel.h"

#include <QtCore/QTimeZone>

QGC_LOGGING_CATEGORY(MAVLinkMessageLog, "AnalyzeView.MAVLinkMessage")

QGCMAVLinkMessage::QGCMAVLinkMessage(const mavlink_message_t &message, const QString &instanceValue, QObject *parent)
    : QObject(parent)
    , _message(message)
    , _fields(new QmlObjectListModel(this))
    , _instanceValue(instanceValue)
{
    qCDebug(MAVLinkMessageLog) << this;

    const mavlink_message_info_t *const msgInfo = mavlink_get_message_info(&message);
    if (!msgInfo) {
        qCWarning(MAVLinkMessageLog) << QStringLiteral("QGCMAVLinkMessage NULL msgInfo msgid(%1)").arg(message.msgid);
        return;
    }

    _name = QString(msgInfo->name);
    if (!_instanceValue.isEmpty()) {
        _name += QStringLiteral(" [%1]").arg(_instanceValue);
    }
    qCDebug(MAVLinkMessageLog) << "New Message:" << _name;

    for (unsigned int i = 0; i < msgInfo->num_fields; ++i) {
        QString type = QStringLiteral("?");
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
            default:
                qCWarning(MAVLinkMessageLog) << "Unknown MAVLink field type:" << msgInfo->fields[i].type;
                break;
        }

        const unsigned int array_length = msgInfo->fields[i].array_length;
        const QString fieldName = QString(msgInfo->fields[i].name);

        if (array_length > 0 && msgInfo->fields[i].type != MAVLINK_TYPE_CHAR) {
            // Expand numeric arrays into individual element fields
            for (unsigned int j = 0; j < array_length; ++j) {
                const QString elementName = QStringLiteral("%1[%2]").arg(fieldName).arg(j);
                QGCMAVLinkMessageField *const field = new QGCMAVLinkMessageField(elementName, type, this);
                _fields->append(field);
                _fieldMappings.append({i, static_cast<int>(j)});
            }
        } else {
            QGCMAVLinkMessageField *const field = new QGCMAVLinkMessageField(fieldName, type, this);
            _fields->append(field);
            _fieldMappings.append({i, -1});
        }
    }
}

QGCMAVLinkMessage::~QGCMAVLinkMessage()
{
    _fields->clearAndDeleteContents();

    qCDebug(MAVLinkMessageLog) << this;
}

QString QGCMAVLinkMessage::extractInstanceValue(const mavlink_message_t &message)
{
    const QString instanceFieldName = mavlinkInstanceFields().value(message.msgid);
    if (instanceFieldName.isEmpty()) {
        // Special-case debug messages that use name/ind as implicit instance discriminator
        return _extractDebugInstanceValue(message);
    }

    const mavlink_message_info_t *const msgInfo = mavlink_get_message_info(&message);
    if (!msgInfo) {
        return QString();
    }

    const uint8_t *const payload = reinterpret_cast<const uint8_t*>(&message.payload64[0]);

    for (unsigned int i = 0; i < msgInfo->num_fields; ++i) {
        if (instanceFieldName != QLatin1String(msgInfo->fields[i].name)) {
            continue;
        }

        const unsigned int offset = msgInfo->fields[i].wire_offset;
        const unsigned int array_length = msgInfo->fields[i].array_length;

        switch (msgInfo->fields[i].type) {
        case MAVLINK_TYPE_CHAR:
            if (array_length > 0) {
                char buf[MAVLINK_MAX_PAYLOAD_LEN + 1]{};
                memcpy(buf, payload + offset, array_length);
                buf[array_length] = '\0';
                return QString::fromLatin1(buf).trimmed();
            } else {
                return QString(QChar::fromLatin1(static_cast<char>(*(payload + offset))));
            }
        case MAVLINK_TYPE_UINT8_T:
            return QString::number(*(payload + offset));
        case MAVLINK_TYPE_INT8_T:
            return QString::number(*reinterpret_cast<const int8_t*>(payload + offset));
        case MAVLINK_TYPE_UINT16_T: {
            uint16_t val = 0;
            memcpy(&val, payload + offset, sizeof(val));
            return QString::number(val);
        }
        case MAVLINK_TYPE_INT16_T: {
            int16_t val = 0;
            memcpy(&val, payload + offset, sizeof(val));
            return QString::number(val);
        }
        case MAVLINK_TYPE_UINT32_T: {
            uint32_t val = 0;
            memcpy(&val, payload + offset, sizeof(val));
            return QString::number(val);
        }
        case MAVLINK_TYPE_INT32_T: {
            int32_t val = 0;
            memcpy(&val, payload + offset, sizeof(val));
            return QString::number(val);
        }
        case MAVLINK_TYPE_UINT64_T: {
            uint64_t val = 0;
            memcpy(&val, payload + offset, sizeof(val));
            return QString::number(val);
        }
        case MAVLINK_TYPE_INT64_T: {
            int64_t val = 0;
            memcpy(&val, payload + offset, sizeof(val));
            return QString::number(val);
        }
        default:
            return QString();
        }
    }

    return QString();
}

QString QGCMAVLinkMessage::_extractDebugInstanceValue(const mavlink_message_t &message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_NAMED_VALUE_FLOAT: {
        char name[MAVLINK_MSG_NAMED_VALUE_FLOAT_FIELD_NAME_LEN + 1]{};
        mavlink_msg_named_value_float_get_name(&message, name);
        name[MAVLINK_MSG_NAMED_VALUE_FLOAT_FIELD_NAME_LEN] = '\0';
        return QString::fromLatin1(name).trimmed();
    }
    case MAVLINK_MSG_ID_NAMED_VALUE_INT: {
        char name[MAVLINK_MSG_NAMED_VALUE_INT_FIELD_NAME_LEN + 1]{};
        mavlink_msg_named_value_int_get_name(&message, name);
        name[MAVLINK_MSG_NAMED_VALUE_INT_FIELD_NAME_LEN] = '\0';
        return QString::fromLatin1(name).trimmed();
    }
    case MAVLINK_MSG_ID_DEBUG_VECT: {
        char name[MAVLINK_MSG_DEBUG_VECT_FIELD_NAME_LEN + 1]{};
        mavlink_msg_debug_vect_get_name(&message, name);
        name[MAVLINK_MSG_DEBUG_VECT_FIELD_NAME_LEN] = '\0';
        return QString::fromLatin1(name).trimmed();
    }
    case MAVLINK_MSG_ID_DEBUG: {
        return QString::number(mavlink_msg_debug_get_ind(&message));
    }
    default:
        return QString();
    }
}

void QGCMAVLinkMessage::updateFieldSelection()
{
    bool sel = false;

    for (int i = 0; i < _fields->count(); ++i) {
        const QGCMAVLinkMessageField *const field = qobject_cast<const QGCMAVLinkMessageField*>(_fields->get(i));
        if (field && field->selected()) {
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
    const qreal lastRateHz = _actualRateHz;
    _actualRateHz = (0.2 * _actualRateHz) + (0.8 * msgCount);
    _lastCount = _count;
    if (_actualRateHz != lastRateHz) {
        emit actualRateHzChanged();
    }
}

void QGCMAVLinkMessage::setSelected(bool sel)
{
    if (sel != _selected) {
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

    if (_selected || _fieldSelected) {
        // Don't update field info unless selected to reduce perf hit of message processing
        _updateFields();
    }
    emit countChanged();
}

void QGCMAVLinkMessage::_updateFields()
{
    const mavlink_message_info_t *msgInfo = mavlink_get_message_info(&_message);
    if (!msgInfo) {
        qCWarning(MAVLinkMessageLog) << "QGCMAVLinkMessage::update NULL msgInfo msgid" << _message.msgid;
        return;
    }

    if (_fields->count() != _fieldMappings.count()) {
        qCWarning(MAVLinkMessageLog) << "QGCMAVLinkMessage::update field mapping count mismatch msgid" << _message.msgid;
        return;
    }

    uint8_t *const msg = reinterpret_cast<uint8_t*>(&_message.payload64[0]);

    for (int idx = 0; idx < _fieldMappings.count(); ++idx) {
        QGCMAVLinkMessageField *const field = qobject_cast<QGCMAVLinkMessageField*>(_fields->get(idx));
        if (!field) {
            continue;
        }

        const FieldMapping &mapping = _fieldMappings.at(idx);
        const unsigned int i = mapping.fieldIndex;
        const int element = mapping.arrayElement;
        const unsigned int offset = msgInfo->fields[i].wire_offset;

        switch (msgInfo->fields[i].type) {
        case MAVLINK_TYPE_CHAR:
            field->setSelectable(false);
            if (msgInfo->fields[i].array_length > 0) {
                char *const str = reinterpret_cast<char*>(msg + offset);
                str[msgInfo->fields[i].array_length - 1] = '\0';
                const QString v(str);
                field->updateValue(v, 0);
            } else {
                char b = *(reinterpret_cast<char*>(msg + offset));
                const QString v(b);
                field->updateValue(v, 0);
            }
            break;
        case MAVLINK_TYPE_UINT8_T:
            if (element >= 0) {
                const uint8_t u = *(msg + offset + element);
                field->updateValue(QString::number(u), static_cast<qreal>(u));
            } else {
                const uint8_t u = *(msg + offset);
                field->updateValue(QString::number(u), static_cast<qreal>(u));
            }
            break;
        case MAVLINK_TYPE_INT8_T:
            if (element >= 0) {
                const int8_t n = *(reinterpret_cast<int8_t*>(msg + offset) + element);
                field->updateValue(QString::number(n), static_cast<qreal>(n));
            } else {
                const int8_t n = *(reinterpret_cast<int8_t*>(msg + offset));
                field->updateValue(QString::number(n), static_cast<qreal>(n));
            }
            break;
        case MAVLINK_TYPE_UINT16_T: {
            uint16_t n = 0;
            if (element >= 0) {
                (void) memcpy(&n, msg + offset + element * static_cast<int>(sizeof(uint16_t)),
                              sizeof(uint16_t));
            } else {
                (void) memcpy(&n, msg + offset, sizeof(uint16_t));
            }
            field->updateValue(QString::number(n), static_cast<qreal>(n));
            break;
        }
        case MAVLINK_TYPE_INT16_T: {
            int16_t n = 0;
            if (element >= 0) {
                (void) memcpy(&n, msg + offset + element * static_cast<int>(sizeof(int16_t)),
                              sizeof(int16_t));
            } else {
                (void) memcpy(&n, msg + offset, sizeof(int16_t));
            }
            field->updateValue(QString::number(n), static_cast<qreal>(n));
            break;
        }
        case MAVLINK_TYPE_UINT32_T: {
            uint32_t n = 0;
            if (element >= 0) {
                (void) memcpy(&n, msg + offset + element * static_cast<int>(sizeof(uint32_t)),
                              sizeof(uint32_t));
            } else {
                (void) memcpy(&n, msg + offset, sizeof(uint32_t));
            }
            if (_message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME && element < 0) {
                const QDateTime d = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(n), QTimeZone::utc());
                field->updateValue(d.toString("HH:mm:ss"), static_cast<qreal>(n));
            } else {
                field->updateValue(QString::number(n), static_cast<qreal>(n));
            }
            break;
        }
        case MAVLINK_TYPE_INT32_T: {
            int32_t n = 0;
            if (element >= 0) {
                (void) memcpy(&n, msg + offset + element * static_cast<int>(sizeof(int32_t)),
                              sizeof(int32_t));
            } else {
                (void) memcpy(&n, msg + offset, sizeof(int32_t));
            }
            field->updateValue(QString::number(n), static_cast<qreal>(n));
            break;
        }
        case MAVLINK_TYPE_FLOAT: {
            float fv = 0;
            if (element >= 0) {
                (void) memcpy(&fv, msg + offset + element * static_cast<int>(sizeof(float)),
                              sizeof(float));
            } else {
                (void) memcpy(&fv, msg + offset, sizeof(float));
            }
            field->updateValue(QString::number(static_cast<double>(fv), 'g', 10), static_cast<qreal>(fv));
            break;
        }
        case MAVLINK_TYPE_DOUBLE: {
            double d = 0;
            if (element >= 0) {
                (void) memcpy(&d, msg + offset + element * static_cast<int>(sizeof(double)),
                              sizeof(double));
            } else {
                (void) memcpy(&d, msg + offset, sizeof(double));
            }
            field->updateValue(QString::number(d, 'g', 15), static_cast<qreal>(d));
            break;
        }
        case MAVLINK_TYPE_UINT64_T: {
            uint64_t n = 0;
            if (element >= 0) {
                (void) memcpy(&n, msg + offset + element * static_cast<int>(sizeof(uint64_t)),
                              sizeof(uint64_t));
            } else {
                (void) memcpy(&n, msg + offset, sizeof(uint64_t));
            }
            if (_message.msgid == MAVLINK_MSG_ID_SYSTEM_TIME && element < 0) {
                const QDateTime d = QDateTime::fromMSecsSinceEpoch(n / 1000, QTimeZone::utc());
                field->updateValue(d.toString("yyyy MM dd HH:mm:ss"), static_cast<qreal>(n));
            } else {
                field->updateValue(QString::number(n), static_cast<qreal>(n));
            }
            break;
        }
        case MAVLINK_TYPE_INT64_T: {
            int64_t n = 0;
            if (element >= 0) {
                (void) memcpy(&n, msg + offset + element * static_cast<int>(sizeof(int64_t)),
                              sizeof(int64_t));
            } else {
                (void) memcpy(&n, msg + offset, sizeof(int64_t));
            }
            field->updateValue(QString::number(n), static_cast<qreal>(n));
            break;
        }
        default:
            qCWarning(MAVLinkMessageLog) << "Unknown MAVLink field type:" << msgInfo->fields[i].type;
            break;
        }
    }
}
