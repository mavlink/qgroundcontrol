/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

 #include "ParamHashCheckStateMachine.h"
#include "WaitForHashCheckParamValueState.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "QGCApplication.h"
#include "ComponentInformationManager.h"
#include "MAVLinkProtocol.h"
#include "QGC.h"
#include "CompInfoParam.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(ParamHashCheckStateMachineLog, "ParameterManager.ParamHashCheckStateMachine")

ParamHashCheckStateMachine::ParamHashCheckStateMachine(Vehicle* vehicle, QState *parentState)
    : QGCStateMachine(QStringLiteral("ParameterManager HASH_CHECK"), vehicle, parentState)
{
    Q_ASSERT(vehicle);

    _setupStateGraph();
}

void ParamHashCheckStateMachine::_setupStateGraph()
{
    auto waitForHashCheckState = new WaitForHashCheckParamValueState(1000, this);   // FIXME: Magic mumber for timeout?
    auto loadCacheState = new FunctionState(QStringLiteral("Parameter hash load cache"), [this](FunctionState */*state*/) { _loadCache(); },  this);
    auto computeHashState = new FunctionState(QStringLiteral("Parameter hash compute"), [this](FunctionState */*state*/) { _computeCacheHash(); }, this);

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

    auto injectState = new FunctionState(QStringLiteral("Parameter hash inject"), [this](FunctionState */*state*/) { _injectCachedParameters(); }, this);
    auto sendAckState = new FunctionState(QStringLiteral("Parameter hash send ack"), [this](FunctionState */*state*/) { _sendHashAckToVehicle(); }, this);
//    auto animateState = new FunctionState(QStringLiteral("Parameter hash animate"), [this](FunctionState */*state*/) {  _startLoadProgressAnimation(); }, this);
    auto mismatchState = new FunctionState(QStringLiteral("Parameter hash mismatch"), [this](FunctionState */*state*/) { _handleCacheMismatchDebug(); }, this);
    auto finalState = new QGCFinalState(this);

    setInitialState(waitForHashCheckState);

    // Normal state transitions
    waitForHashCheckState->addThisTransition(&QGCState::advance, loadCacheState);
    loadCacheState->addThisTransition(&QGCState::advance, computeHashState);
    computeHashState->addThisTransition(&QGCState::advance, evaluateState);
    evaluateState->addThisTransition(&QGCState::advance, injectState);
    injectState->addThisTransition(&QGCState::advance, sendAckState);
    sendAckState->addThisTransition(&QGCState::advance, finalState);

    evaluateState->addTransition(this, &ParamHashCheckStateMachine::cacheLoadFailed, finalState);

    evaluateState->addTransition(this, &ParamHashCheckStateMachine::cacheHashMismatch, mismatchState);
    mismatchState->addThisTransition(&QGCState::advance, finalState);

    waitForHashCheckState->addTransition(waitForHashCheckState, &WaitForHashCheckParamValueState::notFound, finalState);

//     animateState->addThisTransition(&QGCState::advance, finalState);

    connect(waitForHashCheckState, &WaitForHashCheckParamValueState::found, this, [this](int hashCRC) {
        _hashFromVehicle = hashCRC;
    });
}

void ParamHashCheckStateMachine::_loadCache()
{
    qCDebug(ParamHashCheckStateMachineLog) << "Attemping load from cache";

    _cacheMap.clear();
    _computedHash = 0;
    _loadSucceeded = false;
    _hashMatches = false;

    _cacheFilePath = ParameterManager::parameterCacheFile(vehicle()->id(), MAV_COMP_ID_AUTOPILOT1);
    QFile cacheFile(_cacheFilePath);
    if (!cacheFile.exists()) {
        qCDebug(ParamHashCheckStateMachineLog) << "No cache file exists at" << cacheFile.fileName();
        return;
    }

    if (!cacheFile.open(QIODevice::ReadOnly)) {
        qCWarning(ParamHashCheckStateMachineLog) << "Failed to open cache file for reading" << cacheFile.fileName();
        return;
    }

    QDataStream ds(&cacheFile);
    ds >> _cacheMap;

    qCDebug(ParamHashCheckStateMachineLog) << "Loaded" << _cacheMap.count() << "parameters from cache file" << cacheFile.fileName();
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

        FactMetaData* factMeta = vehicle()->compInfoManager()->compInfoParam(MAV_COMP_ID_AUTOPILOT1)->factMetaDataForName(name, factType);
        if (factMeta && factMeta->volatileValue()) {
            qCDebug(ParameterManagerLog) << "Volatile parameter" << name;
            continue;
        }

        const void *const valueData = paramTypeVal.second.constData();
        _computedHash = QGC::crc32(reinterpret_cast<const uint8_t *>(qPrintable(name)), name.length(), _computedHash);
        _computedHash = QGC::crc32(static_cast<const uint8_t *>(valueData), FactMetaData::typeToSize(factType), _computedHash);
    }

    _hashMatches = (_computedHash == _hashFromVehicle);
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
        vehicle()->parameterManager()->_handleParamValue(MAV_COMP_ID_AUTOPILOT1, name, count, index++, mavParamType, paramTypeVal.second);
    }
}

void ParamHashCheckStateMachine::_sendHashAckToVehicle()
{
    if (!_hashMatches) {
        return;
    }

    const SharedLinkInterfacePtr sharedLink = vehicle()->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        return;
    }

    mavlink_param_set_t paramSet{};
    mavlink_param_union_t unionValue{};

    paramSet.param_type = MAV_PARAM_TYPE_UINT32;
    (void) strncpy(paramSet.param_id, "_HASH_CHECK", sizeof(paramSet.param_id));
    unionValue.param_uint32 = _computedHash;
    paramSet.param_value = unionValue.param_float;
    paramSet.target_system = static_cast<uint8_t>(vehicle()->id());
    paramSet.target_component = static_cast<uint8_t>(MAV_COMP_ID_AUTOPILOT1);

    mavlink_message_t message{};
    (void) mavlink_msg_param_set_encode_chan(MAVLinkProtocol::instance()->getSystemId(),
                                             MAVLinkProtocol::getComponentId(),
                                             sharedLink->mavlinkChannel(),
                                             &message,
                                             &paramSet);

    (void) vehicle()->sendMessageOnLinkThreadSafe(sharedLink.get(), message);
}

#if 0
// FIXME: NYI
void ParamHashCheckStateMachine::_startLoadProgressAnimation(QGCState* state)
{
    if (!_hashMatches) {
        return;
    }

    QVariantAnimation *const animation = new QVariantAnimation(_parameterManager);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setDuration(750);

    auto parameterManager = _parameterMan

    (void) QObject::connect(animation, &QVariantAnimation::valueChanged, _parameterManager, [this](const QVariant &value) {
        parameterManager->_setLoadProgress(value.toDouble());
    });

    QObject::connect(animation, &QVariantAnimation::finished, _parameterManager, [this]() {
        QTimer::singleShot(500, _parameterManager, [this]() {
            parameterManager->_setLoadProgress(0);
        });
    });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
#endif

void ParamHashCheckStateMachine::_handleCacheMismatchDebug()
{
    if (!_loadSucceeded || _hashMatches) {
        return;
    }

    qCInfo(ParameterManagerLog) << "Parameters cache match failed" << qPrintable(QFileInfo(_cacheFilePath).absoluteFilePath());

    if (!ParameterManagerDebugCacheFailureLog().isDebugEnabled()) {
        return;
    }

    _debugCacheCRC[_componentId] = true;
    _debugCacheMap[_componentId] = _cacheMap;
    for (auto it = _cacheMap.cbegin(); it != _cacheMap.cend(); ++it) {
        _debugCacheParamSeen[_componentId][it.key()] = false;
    }

    qgcApp()->showAppMessage(QStringLiteral("Parameter cache CRC match failed"));
}
