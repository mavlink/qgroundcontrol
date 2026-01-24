#include "MultiSignalSpy.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaMethod>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(MultiSignalSpyLog, "Test.MultiSignalSpy")

MultiSignalSpy::MultiSignalSpy(QObject* parent)
    : QObject(parent)
{
}

MultiSignalSpy::~MultiSignalSpy()
{
    _cleanup();
}

void MultiSignalSpy::_cleanup()
{
    _spies.clear();
    _signalNames.clear();
    _nameToIndex.clear();
    _signalEmitter = nullptr;
}

void MultiSignalSpy::_onEmitterDestroyed()
{
    qCWarning(MultiSignalSpyLog) << "monitored object was destroyed";
    _cleanup();
}

bool MultiSignalSpy::init(QObject* signalEmitter)
{
    if (!signalEmitter) {
        qCWarning(MultiSignalSpyLog) << "null signalEmitter";
        return false;
    }

    // Clean up any previous monitoring
    _cleanup();

    _signalEmitter = signalEmitter;

    // Connect to destroyed signal to prevent invalid access
    (void) connect(_signalEmitter, &QObject::destroyed, this, &MultiSignalSpy::_onEmitterDestroyed);

    const QMetaObject* metaObject = signalEmitter->metaObject();

    for (int i = 0; i < metaObject->methodCount(); ++i) {
        const QMetaMethod method = metaObject->method(i);
        if (method.methodType() != QMetaMethod::Signal) {
            continue;
        }

        const QString signalName = QString::fromLatin1(method.name());

        // Skip QObject base signals
        if (signalName == QStringLiteral("destroyed") || signalName == QStringLiteral("objectNameChanged")) {
            continue;
        }

        // Skip duplicate signals (can happen with complex inheritance)
        if (_nameToIndex.contains(signalName)) {
            continue;
        }

        // Enforce maximum signal limit (bitmask is quint64)
        if (_signalNames.size() >= kMaxSignals) {
            qCWarning(MultiSignalSpyLog) << "too many signals (max" << kMaxSignals << "), some will not be monitored";
            break;
        }

        const QString signature = QStringLiteral("2%1").arg(QString::fromLatin1(method.methodSignature()));
        auto spy = std::make_unique<QSignalSpy>(_signalEmitter, signature.toLatin1().constData());

        if (!spy->isValid()) {
            qCWarning(MultiSignalSpyLog) << "MultiSignalSpy::init: invalid signal:" << signalName;
            _cleanup();
            return false;
        }

        _nameToIndex[signalName] = _signalNames.size();
        _signalNames.append(signalName);
        _spies.push_back(std::move(spy));
    }

    if (_signalNames.isEmpty()) {
        qCWarning(MultiSignalSpyLog) << "MultiSignalSpy::init: no signals found on object";
        _cleanup();
        return false;
    }

    return true;
}

bool MultiSignalSpy::init(QObject* signalEmitter, const QStringList& signalNames)
{
    if (!signalEmitter) {
        qCWarning(MultiSignalSpyLog) << "MultiSignalSpy::init: null signalEmitter";
        return false;
    }

    if (signalNames.isEmpty()) {
        qCWarning(MultiSignalSpyLog) << "MultiSignalSpy::init: empty signal list";
        return false;
    }

    if (signalNames.size() > kMaxSignals) {
        qCWarning(MultiSignalSpyLog) << "MultiSignalSpy::init: too many signals requested (max" << kMaxSignals << ")";
        return false;
    }

    // Clean up any previous monitoring
    _cleanup();

    _signalEmitter = signalEmitter;

    // Connect to destroyed signal to prevent invalid access
    (void) connect(_signalEmitter, &QObject::destroyed, this, &MultiSignalSpy::_onEmitterDestroyed);

    const QMetaObject* metaObject = signalEmitter->metaObject();

    // Build a lookup map of signal names to method indices for O(1) lookup
    // This avoids O(n*m) complexity when initializing with many signals
    QHash<QString, int> signalMethodMap;
    signalMethodMap.reserve(metaObject->methodCount());
    for (int i = 0; i < metaObject->methodCount(); ++i) {
        const QMetaMethod method = metaObject->method(i);
        if (method.methodType() == QMetaMethod::Signal) {
            signalMethodMap[QString::fromLatin1(method.name())] = i;
        }
    }

    for (const QString& signalName : signalNames) {
        const auto it = signalMethodMap.constFind(signalName);
        if (it == signalMethodMap.constEnd()) {
            qCWarning(MultiSignalSpyLog) << "MultiSignalSpy::init: signal not found:" << signalName;
            _cleanup();
            return false;
        }

        const QMetaMethod method = metaObject->method(it.value());
        const QString signature = QStringLiteral("2%1").arg(QString::fromLatin1(method.methodSignature()));
        auto spy = std::make_unique<QSignalSpy>(_signalEmitter, signature.toLatin1().constData());

        if (!spy->isValid()) {
            qCWarning(MultiSignalSpyLog) << "MultiSignalSpy::init: invalid signal:" << signalName;
            _cleanup();
            return false;
        }

        _nameToIndex[signalName] = _signalNames.size();
        _signalNames.append(signalName);
        _spies.push_back(std::move(spy));
    }

    return true;
}

int MultiSignalSpy::_indexForSignal(const char* signalName) const
{
    if (!_signalEmitter) {
        qCWarning(MultiSignalSpyLog) << " emitter was destroyed, cannot access signal:" << signalName;
        return -1;
    }

    const auto it = _nameToIndex.constFind(QString::fromLatin1(signalName));
    if (it == _nameToIndex.constEnd()) {
        qCWarning(MultiSignalSpyLog) << " signal not found:" << signalName;
        return -1;
    }
    return it.value();
}

quint64 MultiSignalSpy::mask(const char* signalName) const
{
    const int index = _indexForSignal(signalName);
    if (index < 0 || index >= 64) {
        return 0;
    }
    return 1ULL << index;
}

bool MultiSignalSpy::_checkSignalByMaskWorker(quint64 signalMask, bool multipleAllowed) const
{
    if (!_signalEmitter) {
        qCWarning(MultiSignalSpyLog) << " emitter was destroyed, cannot check signals";
        return false;
    }

    for (int i = 0; i < static_cast<int>(_spies.size()); ++i) {
        if (!((1ULL << i) & signalMask)) {
            continue;
        }

        const int signalCount = _spies[i]->count();
        if (multipleAllowed) {
            if (signalCount == 0) {
                qCWarning(MultiSignalSpyLog) << " expected signal not emitted:" << _signalNames[i];
                printState(signalMask);
                return false;
            }
        } else {
            if (signalCount != 1) {
                qCWarning(MultiSignalSpyLog) << " expected exactly 1 emission, got" << signalCount << "for:" << _signalNames[i];
                printState(signalMask);
                return false;
            }
        }
    }
    return true;
}

bool MultiSignalSpy::_checkOnlySignalByMaskWorker(quint64 signalMask, bool multipleAllowed) const
{
    if (!_signalEmitter) {
        qCWarning(MultiSignalSpyLog) << " emitter was destroyed, cannot check signals";
        return false;
    }

    for (int i = 0; i < static_cast<int>(_spies.size()); ++i) {
        const int signalCount = _spies[i]->count();
        const bool expected = (1ULL << i) & signalMask;

        if (expected) {
            if (multipleAllowed) {
                if (signalCount == 0) {
                    qCWarning(MultiSignalSpyLog) << " expected signal not emitted:" << _signalNames[i];
                    printState(signalMask);
                    return false;
                }
            } else {
                if (signalCount != 1) {
                    qCWarning(MultiSignalSpyLog) << " expected exactly 1 emission, got" << signalCount << "for:" << _signalNames[i];
                    printState(signalMask);
                    return false;
                }
            }
        } else {
            if (signalCount != 0) {
                qCWarning(MultiSignalSpyLog) << " unexpected signal emitted:" << _signalNames[i] << "count:" << signalCount;
                printState(signalMask);
                return false;
            }
        }
    }
    return true;
}

bool MultiSignalSpy::checkSignal(const char* signalName) const
{
    return checkSignalByMask(mask(signalName));
}

bool MultiSignalSpy::checkSignals(const char* signalName) const
{
    return checkSignalsByMask(mask(signalName));
}

bool MultiSignalSpy::checkOnlySignal(const char* signalName) const
{
    return checkOnlySignalByMask(mask(signalName));
}

bool MultiSignalSpy::checkOnlySignals(const char* signalName) const
{
    return checkOnlySignalsByMask(mask(signalName));
}

bool MultiSignalSpy::checkNoSignal(const char* signalName) const
{
    return checkNoSignalByMask(mask(signalName));
}

bool MultiSignalSpy::checkNoSignals() const
{
    if (!_signalEmitter) {
        return true; // No signals can be emitted if emitter is gone
    }

    // More efficient: directly check all spies instead of using mask
    for (int i = 0; i < static_cast<int>(_spies.size()); ++i) {
        if (_spies[i]->count() != 0) {
            qCWarning(MultiSignalSpyLog) << " unexpected signal emitted:" << _signalNames[i] << "count:" << _spies[i]->count();
            printState(0);
            return false;
        }
    }
    return true;
}

bool MultiSignalSpy::checkSignalByMask(quint64 signalMask) const
{
    return _checkSignalByMaskWorker(signalMask, false);
}

bool MultiSignalSpy::checkSignalsByMask(quint64 signalMask) const
{
    return _checkSignalByMaskWorker(signalMask, true);
}

bool MultiSignalSpy::checkOnlySignalByMask(quint64 signalMask) const
{
    return _checkOnlySignalByMaskWorker(signalMask, false);
}

bool MultiSignalSpy::checkOnlySignalsByMask(quint64 signalMask) const
{
    return _checkOnlySignalByMaskWorker(signalMask, true);
}

bool MultiSignalSpy::checkNoSignalByMask(quint64 signalMask) const
{
    if (!_signalEmitter) {
        return true; // No signals can be emitted if emitter is gone
    }

    for (int i = 0; i < static_cast<int>(_spies.size()); ++i) {
        if (!((1ULL << i) & signalMask)) {
            continue;
        }
        if (_spies[i]->count() != 0) {
            qCWarning(MultiSignalSpyLog) << " signal should not have been emitted:" << _signalNames[i];
            printState(signalMask);
            return false;
        }
    }
    return true;
}

void MultiSignalSpy::clearSignal(const char* signalName)
{
    const int index = _indexForSignal(signalName);
    if (index >= 0) {
        _spies[index]->clear();
    }
}

void MultiSignalSpy::clearSignalsByMask(quint64 signalMask)
{
    for (int i = 0; i < static_cast<int>(_spies.size()); ++i) {
        if ((1ULL << i) & signalMask) {
            _spies[i]->clear();
        }
    }
}

void MultiSignalSpy::clearAllSignals()
{
    for (auto& spy : _spies) {
        spy->clear();
    }
}

bool MultiSignalSpy::waitForSignal(const char* signalName, int msec)
{
    const int index = _indexForSignal(signalName);
    if (index < 0) {
        return false;
    }

    QSignalSpy* s = _spies[index].get();
    if (s->count() > 0) {
        return true;
    }

    return s->wait(msec);
}

QSignalSpy* MultiSignalSpy::spy(const char* signalName) const
{
    const int index = _indexForSignal(signalName);
    if (index < 0) {
        return nullptr;
    }
    return _spies[index].get();
}

int MultiSignalSpy::count(const char* signalName) const
{
    const QSignalSpy* s = spy(signalName);
    return s ? s->count() : 0;
}

bool MultiSignalSpy::pullBoolFromSignal(const char* signalName)
{
    return argument<bool>(signalName);
}

int MultiSignalSpy::pullIntFromSignal(const char* signalName)
{
    return argument<int>(signalName);
}

QGeoCoordinate MultiSignalSpy::pullQGeoCoordinateFromSignal(const char* signalName)
{
    return argument<QGeoCoordinate>(signalName);
}

void MultiSignalSpy::printState(quint64 expectedMask) const
{
    qCDebug(MultiSignalSpyLog) << "MultiSignalSpy state:";
    for (int i = 0; i < static_cast<int>(_spies.size()); ++i) {
        const bool expected = (1ULL << i) & expectedMask;
        qCDebug(MultiSignalSpyLog) << "  " << _signalNames[i] << "count:" << _spies[i]->count() << (expected ? "(expected)" : "");
    }
}
