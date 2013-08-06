#include "UASParameterDataModel.h"

#include <QDebug>
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

void UASParameterDataModel::setOnboardParameter(int componentId, QString& key,  QVariant& value)
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


void UASParameterDataModel::handleParameterUpdate(int componentId, QString& key, QVariant& value)
{
    //verify that the value requested by the user matches the set value
    //if it doesn't match, leave the pending parameter in the pending list!
    if (pendingParameters.contains(componentId)) {
        QMap<QString , QVariant> *pendingParams = pendingParameters.value(componentId);
        if ((NULL != pendingParams) && pendingParams->contains(key)) {
            QVariant reqVal = pendingParams->value(key);
            if (reqVal == value) {
                pendingParams->remove(key);
            }
            else {
                qDebug() << "Pending commit for " << key << " want: " << reqVal << " got: " << value;
            }
        }
    }

    setOnboardParameter(componentId,key, value);
}
