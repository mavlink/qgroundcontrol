#include "MultiSignalSpy.h"

#include <QtCore/QMetaMethod>

Q_LOGGING_CATEGORY(MultiSignalSpyLog, "Test.MultiSignalSpy")

namespace {
// Normalize signal name from SIGNAL() macro format to plain method name
// Input: "2isCurrentItemChanged(bool)" -> Output: "isCurrentItemChanged"
QString normalizeSignalName(const char* signalName)
{
    QString name = QString::fromLatin1(signalName);

    // Strip Qt signal encoding prefix ("2" for signals)
    if (!name.isEmpty() && name[0].isDigit()) {
        name = name.mid(1);
    }

    // Strip parameter list if present
    const int parenIndex = name.indexOf(QLatin1Char('('));
    if (parenIndex > 0) {
        name = name.left(parenIndex);
    }

    return name;
}
}  // namespace

MultiSignalSpy::MultiSignalSpy(QObject* parent) : QObject(parent)
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
    qCWarning(MultiSignalSpyLog) << "Monitored object destroyed";
    _cleanup();
}

bool MultiSignalSpy::init(QObject* signalEmitter)
{
    if (!signalEmitter) {
        qCWarning(MultiSignalSpyLog) << "Null signalEmitter";
        return false;
    }

    _cleanup();
    _signalEmitter = signalEmitter;

    connect(_signalEmitter, &QObject::destroyed, this, &MultiSignalSpy::_onEmitterDestroyed);

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

        // Skip duplicates from inheritance
        if (_nameToIndex.contains(signalName)) {
            continue;
        }

        if (_signalNames.size() >= kMaxSignals) {
            qCWarning(MultiSignalSpyLog) << "Too many signals (max" << kMaxSignals << ")";
            break;
        }

        const QString signature = QStringLiteral("2%1").arg(QString::fromLatin1(method.methodSignature()));
        auto spy = std::make_unique<QSignalSpy>(_signalEmitter, signature.toLatin1().constData());

        if (!spy->isValid()) {
            qCWarning(MultiSignalSpyLog) << "Invalid signal:" << signalName;
            _cleanup();
            return false;
        }

        _nameToIndex[signalName] = static_cast<int>(_signalNames.size());
        _signalNames.append(signalName);
        _spies.push_back(std::move(spy));
    }

    if (_signalNames.isEmpty()) {
        qCWarning(MultiSignalSpyLog) << "No signals found";
        _cleanup();
        return false;
    }

    return true;
}

bool MultiSignalSpy::init(QObject* signalEmitter, const QStringList& signalNames)
{
    if (!signalEmitter) {
        qCWarning(MultiSignalSpyLog) << "Null signalEmitter";
        return false;
    }

    if (signalNames.isEmpty()) {
        qCWarning(MultiSignalSpyLog) << "Empty signal list";
        return false;
    }

    if (signalNames.size() > kMaxSignals) {
        qCWarning(MultiSignalSpyLog) << "Too many signals (max" << kMaxSignals << ")";
        return false;
    }

    _cleanup();
    _signalEmitter = signalEmitter;

    connect(_signalEmitter, &QObject::destroyed, this, &MultiSignalSpy::_onEmitterDestroyed);

    const QMetaObject* metaObject = signalEmitter->metaObject();

    // Build lookup map for O(1) signal resolution
    QHash<QString, int> signalMethodMap;
    for (int i = 0; i < metaObject->methodCount(); ++i) {
        const QMetaMethod method = metaObject->method(i);
        if (method.methodType() == QMetaMethod::Signal) {
            signalMethodMap[QString::fromLatin1(method.name())] = i;
        }
    }

    for (const QString& signalName : signalNames) {
        const auto it = signalMethodMap.constFind(signalName);
        if (it == signalMethodMap.constEnd()) {
            qCWarning(MultiSignalSpyLog) << "Signal not found:" << signalName;
            _cleanup();
            return false;
        }

        const QMetaMethod method = metaObject->method(it.value());
        const QString signature = QStringLiteral("2%1").arg(QString::fromLatin1(method.methodSignature()));
        auto spy = std::make_unique<QSignalSpy>(_signalEmitter, signature.toLatin1().constData());

        if (!spy->isValid()) {
            qCWarning(MultiSignalSpyLog) << "Invalid signal:" << signalName;
            _cleanup();
            return false;
        }

        _nameToIndex[signalName] = static_cast<int>(_signalNames.size());
        _signalNames.append(signalName);
        _spies.push_back(std::move(spy));
    }

    return true;
}

int MultiSignalSpy::_indexForSignal(const char* signalName) const
{
    if (!_signalEmitter) {
        return -1;
    }

    // Normalize to handle both "isCurrentItemChanged" and "2isCurrentItemChanged(bool)"
    const QString normalizedName = normalizeSignalName(signalName);
    const auto it = _nameToIndex.constFind(normalizedName);
    if (it == _nameToIndex.constEnd()) {
        qCWarning(MultiSignalSpyLog) << "Signal not monitored:" << signalName;
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

bool MultiSignalSpy::_emittedOnceByMaskWorker(quint64 signalMask, bool multipleAllowed) const
{
    if (!_signalEmitter) {
        return false;
    }

    for (size_t i = 0; i < _spies.size(); ++i) {
        if (!((1ULL << i) & signalMask)) {
            continue;
        }

        const int signalCount = _spies[i]->count();
        if (multipleAllowed) {
            if (signalCount == 0) {
                return false;
            }
        } else {
            if (signalCount != 1) {
                return false;
            }
        }
    }
    return true;
}

bool MultiSignalSpy::_onlyEmittedOnceByMaskWorker(quint64 signalMask, bool multipleAllowed) const
{
    if (!_signalEmitter) {
        return false;
    }

    for (size_t i = 0; i < _spies.size(); ++i) {
        const int signalCount = _spies[i]->count();
        const bool expected = (1ULL << i) & signalMask;

        if (expected) {
            if (multipleAllowed) {
                if (signalCount == 0) {
                    return false;
                }
            } else {
                if (signalCount != 1) {
                    return false;
                }
            }
        } else {
            if (signalCount != 0) {
                return false;
            }
        }
    }
    return true;
}

bool MultiSignalSpy::emittedOnce(const char* signalName) const
{
    return emittedOnceByMask(mask(signalName));
}

bool MultiSignalSpy::emitted(const char* signalName) const
{
    return emittedByMask(mask(signalName));
}

bool MultiSignalSpy::onlyEmittedOnce(const char* signalName) const
{
    return onlyEmittedOnceByMask(mask(signalName));
}

bool MultiSignalSpy::onlyEmitted(const char* signalName) const
{
    return onlyEmittedByMask(mask(signalName));
}

bool MultiSignalSpy::notEmitted(const char* signalName) const
{
    return notEmittedByMask(mask(signalName));
}

bool MultiSignalSpy::noneEmitted() const
{
    if (!_signalEmitter) {
        return true;
    }

    for (size_t i = 0; i < _spies.size(); ++i) {
        if (_spies[i]->count() != 0) {
            return false;
        }
    }
    return true;
}

bool MultiSignalSpy::emittedOnceByMask(quint64 signalMask) const
{
    return _emittedOnceByMaskWorker(signalMask, false);
}

bool MultiSignalSpy::emittedByMask(quint64 signalMask) const
{
    return _emittedOnceByMaskWorker(signalMask, true);
}

bool MultiSignalSpy::onlyEmittedOnceByMask(quint64 signalMask) const
{
    return _onlyEmittedOnceByMaskWorker(signalMask, false);
}

bool MultiSignalSpy::onlyEmittedByMask(quint64 signalMask) const
{
    return _onlyEmittedOnceByMaskWorker(signalMask, true);
}

bool MultiSignalSpy::notEmittedByMask(quint64 signalMask) const
{
    if (!_signalEmitter) {
        return true;
    }

    for (size_t i = 0; i < _spies.size(); ++i) {
        if (!((1ULL << i) & signalMask)) {
            continue;
        }
        if (_spies[i]->count() != 0) {
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
    for (size_t i = 0; i < _spies.size(); ++i) {
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

    const QString normalizedName = normalizeSignalName(signalName);
    return UnitTest::waitForSignal(*s, msec, normalizedName);
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

bool MultiSignalSpy::hasSignal(const char* signalName) const
{
    const QString normalizedName = normalizeSignalName(signalName);
    return _nameToIndex.contains(normalizedName);
}

QString MultiSignalSpy::summary() const
{
    QStringList parts;
    for (int i = 0; i < static_cast<int>(_spies.size()); ++i) {
        const int c = _spies[i]->count();
        if (c > 0) {
            parts.append(QStringLiteral("%1=%2").arg(_signalNames[i]).arg(c));
        }
    }
    return parts.isEmpty() ? QStringLiteral("(no signals)") : parts.join(QStringLiteral(", "));
}

int MultiSignalSpy::totalEmissions() const
{
    int total = 0;
    for (const auto& spy : _spies) {
        total += spy->count();
    }
    return total;
}

int MultiSignalSpy::uniqueSignalsEmitted() const
{
    int unique = 0;
    for (const auto& spy : _spies) {
        if (spy->count() > 0) {
            ++unique;
        }
    }
    return unique;
}

void MultiSignalSpy::printState(quint64 expectedMask) const
{
    qCDebug(MultiSignalSpyLog) << "Signal state:";
    for (int i = 0; i < static_cast<int>(_spies.size()); ++i) {
        const bool expected = (1ULL << i) & expectedMask;
        qCDebug(MultiSignalSpyLog) << " " << _signalNames[i] << "count:" << _spies[i]->count()
                                   << (expected ? "(expected)" : "");
    }
}
