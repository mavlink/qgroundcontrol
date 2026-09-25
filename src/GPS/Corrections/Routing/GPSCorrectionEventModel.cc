#include "GPSCorrectionEventModel.h"

#include <algorithm>
#include <utility>

#include <QtCore/QPointer>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSCorrectionEventModelLog, "GPS.Corrections.GPSCorrectionEventModel")

// A const range keeps the model read-only; the base only records its address and reads rows after construction.
GPSCorrectionEventModel::GPSCorrectionEventModel(QObject* parent)
    : QRangeModel(&std::as_const(_events), parent)
{
    qCDebug(GPSCorrectionEventModelLog) << this;
}

GPSCorrectionEventModel::~GPSCorrectionEventModel()
{
    qCDebug(GPSCorrectionEventModelLog) << this;
}

void GPSCorrectionEventModel::setEvents(const QList<GPSCorrectionEvent>& events)
{
    _pendingEvents = events.last((std::min) (events.size(), GPS_CORRECTION_MAX_EVENTS));
    _pending = true;
    if (_updating) {
        return;
    }
    _updating = true;
    const QPointer<GPSCorrectionEventModel> guard(this);
    while (_pending) {
        _pending = false;
        const auto snapshot = _pendingEvents;
        qsizetype remove = 0;
        while (remove < _events.size() &&
               (snapshot.isEmpty() || _events[remove].sequence != snapshot.first().sequence)) {
            ++remove;
        }
        qsizetype retained = _events.size() - remove;
        if (retained > snapshot.size()) {
            remove = _events.size();
            retained = 0;
        }
        for (qsizetype index = 0; index < retained; ++index) {
            if (_events[remove + index].sequence != snapshot[index].sequence) {
                remove = _events.size();
                retained = 0;
                break;
            }
        }
        if (remove > 0) {
            beginRemoveRows({}, 0, static_cast<int>(remove - 1));
            if (!guard) {
                return;
            }
            _events.remove(0, remove);
            endRemoveRows();
            if (!guard) {
                return;
            }
        }
        if (retained < snapshot.size()) {
            beginInsertRows({}, static_cast<int>(retained), static_cast<int>(snapshot.size() - 1));
            if (!guard) {
                return;
            }
            _events.append(snapshot.sliced(retained));
            endInsertRows();
            if (!guard) {
                return;
            }
        }
    }
    _updating = false;
}
