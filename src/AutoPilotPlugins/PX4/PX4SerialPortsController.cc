#include "PX4SerialPortsController.h"

#include <algorithm>

#include "Fact.h"
#include "PX4SerialPortsComponent.h"
#include "ParameterManager.h"
#include "QmlObjectListModel.h"
#include "Vehicle.h"

namespace {

QList<PX4SerialPortConfig::Function> makeFunctions(const QList<Fact*>& functionFacts)
{
    QList<PX4SerialPortConfig::Function> functions;

    for (const Fact* fact : functionFacts) {
        functions.append(PX4SerialPortConfig::makeFunction(fact->name(), fact->shortDescription(), fact->enumValues(),
                                                           fact->enumStrings(), fact->rawValue().toInt()));
    }

    return functions;
}

}  // namespace

PX4SerialPortRow::PX4SerialPortRow(int portId, const QString& label, const QStringList& functionParamNames,
                                   const QStringList& functionLabels, QObject* parent)
    : QObject(parent)
    , _portId(portId)
    , _label(label)
    , _functionParamNames(functionParamNames)
    , _functionLabels(functionLabels)
    , _shared(PX4SerialPortConfig::isSharedPort(portId))
{}

QString PX4SerialPortRow::functionParamName(int functionIndex) const
{
    return _functionParamNames.value(_noneAvailable ? functionIndex : (functionIndex + 1));
}

int PX4SerialPortRow::functionIndexOf(const QString& functionParamName) const
{
    const int index = static_cast<int>(_functionParamNames.indexOf(functionParamName));
    return (_noneAvailable || (index < 0)) ? index : (index - 1);
}

void PX4SerialPortRow::setNoneAvailable(bool noneAvailable)
{
    if (noneAvailable != _noneAvailable) {
        _noneAvailable = noneAvailable;
        emit functionLabelsChanged();
    }
}

void PX4SerialPortRow::setFunctionIndex(int functionIndex)
{
    if (functionIndex != _functionIndex) {
        _functionIndex = functionIndex;
        emit functionIndexChanged();
    }
}

void PX4SerialPortRow::setFunctionEnabled(const QVariantList& functionEnabled)
{
    if (functionEnabled != _functionEnabled) {
        _functionEnabled = functionEnabled;
        emit functionEnabledChanged();
    }
}

void PX4SerialPortRow::setConflictText(const QString& conflictText)
{
    if (conflictText != _conflictText) {
        _conflictText = conflictText;
        emit conflictTextChanged();
    }
}

void PX4SerialPortRow::setBaudFact(Fact* baudFact)
{
    if (baudFact != _baudFact) {
        _baudFact = baudFact;
        emit baudFactChanged();
    }
}

void PX4SerialPortRow::setBaudLoading(bool baudLoading)
{
    if (baudLoading != _baudLoading) {
        _baudLoading = baudLoading;
        emit baudLoadingChanged();
    }
}

void PX4SerialPortRow::setOptionFacts(const QVariantList& optionFacts)
{
    if (optionFacts != _optionFacts) {
        _optionFacts = optionFacts;
        emit optionFactsChanged();
    }
}

void PX4SerialPortRow::setOptionsLoading(bool optionsLoading)
{
    if (optionsLoading != _optionsLoading) {
        _optionsLoading = optionsLoading;
        emit optionsLoadingChanged();
    }
}

void PX4SerialPortRow::setRebootRequired(bool rebootRequired)
{
    if (rebootRequired != _rebootRequired) {
        _rebootRequired = rebootRequired;
        emit rebootRequiredChanged();
    }
}

PX4SerialPortsController::PX4SerialPortsController(QObject* parent)
    : FactPanelController(parent)
    , _ports(new QmlObjectListModel(this))
{
    if (!_vehicle) {
        return;
    }
    _parameterManager = _vehicle->parameterManager();
    if (!_parameterManager) {
        return;
    }

    _parameterManager->suppressRebootMessage();
    (void) connect(_parameterManager, &ParameterManager::factAdded, this, &PX4SerialPortsController::_factAdded);
    (void) connect(_parameterManager, &ParameterManager::_paramRequestReadFailure, this,
                   &PX4SerialPortsController::_parameterReadFailed);

    _functionFacts = PX4SerialPortsComponent::functionFacts(_parameterManager);
    for (Fact* fact : _functionFacts) {
        _watchFact(fact);
    }

    const QList<PX4SerialPortConfig::Function> functions = makeFunctions(_functionFacts);
    for (const PX4SerialPortConfig::Port& port : PX4SerialPortConfig::ports(functions)) {
        QStringList functionParamNames{QString()};
        QStringList functionLabels{tr("None")};
        for (const PX4SerialPortConfig::Function& function :
             PX4SerialPortConfig::functionsForPort(functions, port.id)) {
            functionParamNames.append(function.paramName);
            functionLabels.append(function.label);
        }
        _ports->append(new PX4SerialPortRow(port.id, port.label, functionParamNames, functionLabels, this));
    }

    _update();
}

PX4SerialPortsController::~PX4SerialPortsController()
{
    if (_parameterManager) {
        _parameterManager->unsuppressRebootMessage();
    }
}

void PX4SerialPortsController::setPortFunction(int portId, int functionIndex)
{
    const PX4SerialPortRow* const row = _row(portId);
    if (!_parameterManager || !row || row->shared()) {
        return;
    }

    _writeParams(
        PX4SerialPortConfig::assignPort(makeFunctions(_functionFacts), portId, row->functionParamName(functionIndex)));
}

void PX4SerialPortsController::setPortFunctionEnabled(int portId, int functionIndex, bool enabled)
{
    const PX4SerialPortRow* const row = _row(portId);
    if (!_parameterManager || !row || !row->shared()) {
        return;
    }

    const QList<PX4SerialPortConfig::Function> functions = makeFunctions(_functionFacts);
    const QString functionParamName = row->functionParamName(functionIndex);
    _writeParams(enabled ? PX4SerialPortConfig::assignPort(functions, portId, functionParamName)
                         : PX4SerialPortConfig::removeFromPort(functions, portId, functionParamName));
}

void PX4SerialPortsController::revertChanges()
{
    // An earlier write still retrying could land after the revert and undo it
    if (!_parameterManager || _parameterManager->pendingWrites()) {
        return;
    }

    // Each write updates the rows, which can add newly seen parameters to _initialValues
    const QHash<QString, QVariant> initialValues = _initialValues;
    for (auto it = initialValues.cbegin(); it != initialValues.cend(); ++it) {
        if (_differsFromInitial(it.key())) {
            _fact(it.key())->setRawValue(it.value());
        }
    }
}

void PX4SerialPortsController::_update()
{
    // The facts belong to the parameter manager, which may be gone if the vehicle disconnected
    if (!_parameterManager) {
        return;
    }

    const QList<PX4SerialPortConfig::Function> functions = makeFunctions(_functionFacts);
    QList<QStringList> rowsParams;  // Per row, parameters whose change marks it Reboot required

    // Options can move to another row, so every row records the options it shows before any row is checked
    for (int i = 0; i < _ports->count(); i++) {
        PX4SerialPortRow* const row = _ports->value<PX4SerialPortRow*>(i);
        const int portId = row->portId();
        const QStringList onPort = PX4SerialPortConfig::functionsOnPort(functions, portId);
        QStringList rowParams;

        // Choosing "None" can't remove a function from its default port
        const bool defaultFunctionHere =
            std::any_of(functions.cbegin(), functions.cend(), [portId](const PX4SerialPortConfig::Function& f) {
                return (f.defaultPortId == portId) && (f.currentPortId == portId);
            });
        row->setNoneAvailable(!defaultFunctionHere);
        row->setFunctionIndex(onPort.isEmpty() ? 0 : row->functionIndexOf(onPort.first()));

        QVariantList functionEnabled;
        for (int functionIndex = 0; functionIndex < row->functionLabels().count(); functionIndex++) {
            const QString paramName = row->functionParamName(functionIndex);
            functionEnabled.append(paramName.isEmpty() ? onPort.isEmpty() : onPort.contains(paramName));
        }
        row->setFunctionEnabled(functionEnabled);

        QString conflictText;
        if ((onPort.count() > 1) && !row->shared()) {
            const auto defaultsHere = std::find_if(
                functions.cbegin(), functions.cend(), [&onPort, portId](const PX4SerialPortConfig::Function& f) {
                    return (f.defaultPortId == portId) && onPort.contains(f.paramName);
                });
            QStringList labels;
            for (const QString& paramName : onPort) {
                if ((defaultsHere == functions.cend()) || (paramName != defaultsHere->paramName)) {
                    labels.append(row->functionLabels().value(row->functionIndexOf(paramName)));
                }
            }
            if (defaultsHere != functions.cend()) {
                // Disabling the function doesn't stop it, so the usual "only one will start" doesn't apply
                conflictText = tr("%1 always uses this port unless it is assigned to another port, so %2 will not work "
                                  "here.")
                                   .arg(defaultsHere->label, labels.join(", "));
            } else {
                conflictText =
                    (labels.count() == 2)
                        ? tr("%1 and %2 are both assigned to this port. Only one of them will start.")
                              .arg(labels[0], labels[1])
                        : tr("%1 are all assigned to this port. Only one of them will start.").arg(labels.join(", "));
            }
        }
        row->setConflictText(conflictText);

        Fact* baudFact = nullptr;
        const QString baudParamName = PX4SerialPortConfig::baudParamName(portId);
        if (!baudParamName.isEmpty()) {
            baudFact = _fact(baudParamName);
            if (baudFact) {
                rowParams.append(baudParamName);
            } else {
                _requestParameter(baudParamName);
            }
        }
        row->setBaudFact(baudFact);
        row->setBaudLoading(!baudFact && _pendingParams.contains(baudParamName));

        QVariantList optionFacts;
        bool optionsLoading = false;
        const QStringList optionFunctions = (row->shared() || (onPort.count() == 1)) ? onPort : QStringList();
        for (const QString& functionParamName : optionFunctions) {
            for (const QString& paramName : PX4SerialPortConfig::optionParamNames(functionParamName, portId)) {
                if (Fact* const fact = _fact(paramName)) {
                    optionFacts.append(QVariant::fromValue(fact));
                    _optionParamPorts.insert(paramName, portId);
                } else {
                    _requestParameter(paramName);
                    optionsLoading = optionsLoading || _pendingParams.contains(paramName);
                }
            }
        }
        row->setOptionFacts(optionFacts);
        row->setOptionsLoading(optionsLoading);

        rowsParams.append(rowParams);
    }

    int changedPortCount = 0;
    for (int i = 0; i < _ports->count(); i++) {
        PX4SerialPortRow* const row = _ports->value<PX4SerialPortRow*>(i);
        const int portId = row->portId();
        QStringList rowParams = rowsParams[i];

        for (auto it = _optionParamPorts.cbegin(); it != _optionParamPorts.cend(); ++it) {
            if (it.value() == portId) {
                rowParams.append(it.key());
            }
        }

        for (const PX4SerialPortConfig::Function& function : functions) {
            const int initialValue = _initialValues.value(function.paramName).toInt();
            const int initialPortId =
                (initialValue == PX4SerialPortConfig::kDisabledPortId) ? function.defaultPortId : initialValue;
            if ((function.currentPortId == portId) || (initialPortId == portId)) {
                rowParams.append(function.paramName);
            }
        }
        const bool rebootRequired = std::any_of(rowParams.cbegin(), rowParams.cend(), [this](const QString& paramName) {
            return _differsFromInitial(paramName);
        });
        row->setRebootRequired(rebootRequired);
        if (rebootRequired) {
            changedPortCount++;
        }
    }

    if (changedPortCount != _changedPortCount) {
        _changedPortCount = changedPortCount;
        emit changedPortCountChanged();
    }
}

void PX4SerialPortsController::_factAdded(int componentId, Fact* fact)
{
    if (!_vehicle || !fact || (componentId != _vehicle->defaultComponentId()) || !_pendingParams.remove(fact->name())) {
        return;
    }

    // factAdded is emitted before the new fact has its value from the vehicle
    (void) QMetaObject::invokeMethod(this, &PX4SerialPortsController::_update, Qt::QueuedConnection);
}

void PX4SerialPortsController::_parameterReadFailed(int componentId, const QString& paramName)
{
    if (!_vehicle || (componentId != _vehicle->defaultComponentId()) || !_pendingParams.remove(paramName)) {
        return;
    }

    _update();
}

Fact* PX4SerialPortsController::_fact(const QString& paramName)
{
    if (!_parameterManager || !_parameterManager->parameterExists(ParameterManager::defaultComponentId, paramName)) {
        return nullptr;
    }

    Fact* const fact = _parameterManager->getParameter(ParameterManager::defaultComponentId, paramName);
    _watchFact(fact);
    return fact;
}

void PX4SerialPortsController::_watchFact(Fact* fact)
{
    if (!fact || _watchedFacts.contains(fact)) {
        return;
    }

    _watchedFacts.insert(fact);
    _initialValues.insert(fact->name(), fact->rawValue());
    (void) connect(fact, &Fact::rawValueChanged, this, &PX4SerialPortsController::_update);
}

void PX4SerialPortsController::_requestParameter(const QString& paramName)
{
    if (!_parameterManager || _requestedParams.contains(paramName)) {
        return;
    }

    _requestedParams.insert(paramName);
    _pendingParams.insert(paramName);
    _parameterManager->refreshParameter(ParameterManager::defaultComponentId, paramName, false /* notifyFailure */);
}

bool PX4SerialPortsController::_differsFromInitial(const QString& paramName) const
{
    const auto initial = _initialValues.constFind(paramName);
    if ((initial == _initialValues.cend()) || !_parameterManager ||
        !_parameterManager->parameterExists(ParameterManager::defaultComponentId, paramName)) {
        return false;
    }

    const Fact* const fact = _parameterManager->getParameter(ParameterManager::defaultComponentId, paramName);
    return fact && (fact->rawValue() != *initial);
}

void PX4SerialPortsController::_writeParams(const QList<PX4SerialPortConfig::ParamWrite>& writes)
{
    for (const PX4SerialPortConfig::ParamWrite& write : writes) {
        if (Fact* const fact = _fact(write.paramName)) {
            fact->setRawValue(write.value);
        }
    }
}

PX4SerialPortRow* PX4SerialPortsController::_row(int portId) const
{
    for (int i = 0; i < _ports->count(); i++) {
        PX4SerialPortRow* const row = _ports->value<PX4SerialPortRow*>(i);
        if (row->portId() == portId) {
            return row;
        }
    }
    return nullptr;
}
