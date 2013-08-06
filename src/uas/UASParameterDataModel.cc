#include "UASParameterDataModel.h"

#include <QVariant>

UASParameterDataModel::UASParameterDataModel(QObject *parent) :
    QObject(parent)
{


}

void UASParameterDataModel::addPendingIfParameterChanged(int componentId, QString& key,  QVariant &value)
{
    addComponent(componentId);
    QMap<QString, QVariant> *existParams = getOnbardParametersForComponent(componentId);
    QMap<QString, QVariant> *pendParams = getPendingParametersForComponent(componentId);

    QVariant existValue = existParams->value(key);
    QVariant pendValue = pendParams->value(key);
    if (!(existValue == pendValue)) {
        setPendingParameter(componentId,key,value);
    }
}


void UASParameterDataModel::setPendingParameter(int componentId, QString& key,  QVariant &value)
{
    //ensure we have a placeholder map for this component
    addComponent(componentId);
    QMap<QString, QVariant> *params = getPendingParametersForComponent(componentId);
    params->insert(key,value);
}

void UASParameterDataModel::setOnboardParameter(int componentId, QString& key,  QVariant &value)
{
    //ensure we have a placeholder map for this component
    addComponent(componentId);
    QMap<QString, QVariant> *params = getOnbardParametersForComponent(componentId);
    params->insert(key,value);
}

void UASParameterDataModel::addComponent(int componentId)
{
    if (!onboardParameters.contains(componentId)) {
        onboardParameters.insert(componentId, new QMap<QString, QVariant>());
    }
    if (!pendingParameters.contains(componentId)) {
        pendingParameters.insert(componentId, new QMap<QString, QVariant>());
    }
}
