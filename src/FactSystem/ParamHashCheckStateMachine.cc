#include "ParamHashCheckStateMachine.h"

#include "ParameterManager.h"

#include "ComponentInformationManager.h"
#include "FactMetaData.h"
#include "MAVLinkProtocol.h"
#include "QGC.h"
#include "QGCApplication.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QDataStream>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QAbstractAnimation>
#include <QtCore/QVariantAnimation>
#include <QtCore/QEasingCurve>

#include <cstring>

ParamHashCheckStateMachine::ParamHashCheckStateMachine(ParameterManager* parameterManager, int vehicleId, int componentId, const QVariant& hashValue)
    : QGCStateMachine(QStringLiteral("ParameterManager HASH_CHECK"), parameterManager->vehicle(), parameterManager)
    , _parameterManager(parameterManager)
    , _vehicleId(vehicleId)
    , _componentId(componentId)
    , _hashValue(hashValue)
{
    Q_ASSERT(parameterManager);

    _setupStateGraph();
}

void ParamHashCheckStateMachine::_setupStateGraph()
{
    auto waitForHashCheckState = new WaitForHashCheckParamValue(this, 1000);
    auto loadCacheState = new FunctionState(QStringLiteral("Parameter hash load cache"), this, [this]() {
        _loadCache();
    });
    auto computeHashState = new FunctionState(QStringLiteral("Parameter hash compute"), this, [this]() {
        _computeCacheHash();
    });

    auto evaluateState = new QGCState(QStringLiteral("Parameter hash evaluate"), this);
    connect(evaluateState, &QState::entered, evaluateState, [this, evaluateState]() {
        if (!_loadSucceeded) {
            emit cacheLoadFailed();
            return;
        }

        if (!_hashMatches) {
            emit cacheHashMismatch();
            return;
        }

        evaluateState->advance();
    });

    auto injectState = new FunctionState(QStringLiteral("Parameter hash inject"), this, [this]() {
        _injectCachedParameters();
    });
    auto sendAckState = new FunctionState(QStringLiteral("Parameter hash send ack"), this, [this]() {
        _sendHashAckToVehicle();
    });
    auto animateState = new FunctionState(QStringLiteral("Parameter hash animate"), this, [this]() {
        _startLoadProgressAnimation();
    });
    auto mismatchState = new FunctionState(QStringLiteral("Parameter hash mismatch"), this, [this]() {
        _handleCacheMismatchDebug();
    });
    auto finalState = new QGCFinalState(this);

    setInitialState(waitForHashCheckState);

    waitForHashCheckState->addThisTransition(&QGCState::advance, loadCacheState);
    loadCacheState->addThisTransition(&QGCState::advance, computeHashState);
    computeHashState->addThisTransition(&QGCState::advance, evaluateState);

    evaluateState->addTransition(this, &ParamHashCheckStateMachine::cacheLoadFailed, finalState);
    evaluateState->addTransition(this, &ParamHashCheckStateMachine::cacheHashMismatch, mismatchState);
    evaluateState->addThisTransition(&QGCState::advance, injectState);

    mismatchState->addThisTransition(&QGCState::advance, finalState);

    injectState->addThisTransition(&QGCState::advance, sendAckState);
    sendAckState->addThisTransition(&QGCState::advance, animateState);
    animateState->addThisTransition(&QGCState::advance, finalState);
}

void ParamHashCheckStateMachine::_loadCache()
{
    qCInfo(ParameterManagerLog) << "Attemping load from cache";

    _cacheMap.clear();
    _computedHash = 0;
    _loadSucceeded = false;
    _hashMatches = false;

    _cacheFilePath = ParameterManager::parameterCacheFile(_vehicleId, _componentId);
    QFile cacheFile(_cacheFilePath);
    if (!cacheFile.exists()) {
        return;
    }

    if (!cacheFile.open(QIODevice::ReadOnly)) {
        qCWarning(ParameterManagerLog) << "Failed to open cache file for reading" << cacheFile.fileName();
        return;
    }

    QDataStream ds(&cacheFile);
    ds >> _cacheMap;

    _loadSucceeded = true;
}

void ParamHashCheckStateMachine::_computeCacheHash()
{
    if (!_loadSucceeded) {
        _hashMatches = false;
        return;
    }

    _computedHash = 0;

    for (auto it = _cacheMap.cbegin(); it != _cacheMap.cend(); ++it) {
        const QString& name = it.key();
        const ParamTypeVal& paramTypeVal = it.value();
        const FactMetaData::ValueType_t factType = static_cast<FactMetaData::ValueType_t>(paramTypeVal.first);

        FactMetaData* factMeta = _parameterManager->_vehicle->compInfoManager()->compInfoParam(_componentId)->factMetaDataForName(name, factType);
        if (factMeta && factMeta->volatileValue()) {
            qCDebug(ParameterManagerLog) << "Volatile parameter" << name;
            continue;
        }

        const void *const valueData = paramTypeVal.second.constData();
        _computedHash = QGC::crc32(reinterpret_cast<const uint8_t *>(qPrintable(name)), name.length(), _computedHash);
        _computedHash = QGC::crc32(static_cast<const uint8_t *>(valueData), FactMetaData::typeToSize(factType), _computedHash);
    }

    _hashMatches = (_computedHash == _hashValue.toUInt());
}

void ParamHashCheckStateMachine::_injectCachedParameters()
{
    if (!_hashMatches) {
        return;
    }

    if (_cacheMap.isEmpty()) {
        return;
    }

    qCInfo(ParameterManagerLog) << "Parameters loaded from cache" << qPrintable(QFileInfo(_cacheFilePath).absoluteFilePath());

    const int count = _cacheMap.count();
    int index = 0;
    for (auto it = _cacheMap.cbegin(); it != _cacheMap.cend(); ++it) {
        const QString& name = it.key();
        const ParamTypeVal& paramTypeVal = it.value();
        const FactMetaData::ValueType_t factType = static_cast<FactMetaData::ValueType_t>(paramTypeVal.first);
        const MAV_PARAM_TYPE mavParamType = ParameterManager::factTypeToMavType(factType);
        _parameterManager->_handleParamValue(_componentId, name, count, index++, mavParamType, paramTypeVal.second);
    }
}

void ParamHashCheckStateMachine::_sendHashAckToVehicle()
{
    if (!_hashMatches) {
        return;
    }

    const SharedLinkInterfacePtr sharedLink = _parameterManager->_vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    mavlink_param_set_t paramSet{};
    mavlink_param_union_t unionValue{};

    paramSet.param_type = MAV_PARAM_TYPE_UINT32;
    (void) strncpy(paramSet.param_id, "_HASH_CHECK", sizeof(paramSet.param_id));
    unionValue.param_uint32 = _computedHash;
    paramSet.param_value = unionValue.param_float;
    paramSet.target_system = static_cast<uint8_t>(_parameterManager->_vehicle->id());
    paramSet.target_component = static_cast<uint8_t>(_componentId);

    mavlink_message_t message{};
    (void) mavlink_msg_param_set_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                             MAVLinkProtocol::getComponentId(),
                                             sharedLink->mavlinkChannel(),
                                             &message,
                                             &paramSet);

    (void) _parameterManager->_vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
}

void ParamHashCheckStateMachine::_startLoadProgressAnimation()
{
    if (!_hashMatches) {
        return;
    }

    QVariantAnimation *const animation = new QVariantAnimation(_parameterManager);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setDuration(750);

    (void) QObject::connect(animation, &QVariantAnimation::valueChanged, _parameterManager, [this](const QVariant &value) {
        _parameterManager->_setLoadProgress(value.toDouble());
    });

    QObject::connect(animation, &QVariantAnimation::finished, _parameterManager, [this]() {
        QTimer::singleShot(500, _parameterManager, [this]() {
            _parameterManager->_setLoadProgress(0);
        });
    });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ParamHashCheckStateMachine::_handleCacheMismatchDebug()
{
    if (!_loadSucceeded || _hashMatches) {
        return;
    }

    qCInfo(ParameterManagerLog) << "Parameters cache match failed" << qPrintable(QFileInfo(_cacheFilePath).absoluteFilePath());

    if (!ParameterManagerDebugCacheFailureLog().isDebugEnabled()) {
        return;
    }

    _parameterManager->_debugCacheCRC[_componentId] = true;
    _parameterManager->_debugCacheMap[_componentId] = _cacheMap;
    for (auto it = _cacheMap.cbegin(); it != _cacheMap.cend(); ++it) {
        _parameterManager->_debugCacheParamSeen[_componentId][it.key()] = false;
    }

    qgcApp()->showAppMessage(_parameterManager->tr("Parameter cache CRC match failed"));
}
