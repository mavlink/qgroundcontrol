#include "UASParameterDataModel.h"

#include <QVariant>

UASParameterDataModel::UASParameterDataModel(QObject *parent) :
    QObject(parent)
{





}


void UASParameterDataModel::setPendingParameter(int componentId, QString& key,  QVariant &value)
{
    QMap<QString, QVariant> *compPendingParams = pendingParameters.value(componentId);
    //TODO insert blank map if necessary
    if (NULL == compPendingParams) {
        pendingParameters.insert(componentId,new QMap<QString, QVariant>());
        compPendingParams = pendingParameters.value(componentId);
    }

    compPendingParams->insert(key,value);
}
