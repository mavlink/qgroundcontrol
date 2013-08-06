#include "UASParameterDataModel.h"

#include <float.h>

#include <QDebug>
#include <QStringList>
#include <QVariant>

#include "QGCMAVLink.h"

UASParameterDataModel::UASParameterDataModel(QObject *parent) :
    QObject(parent)
{


}



bool UASParameterDataModel::checkParameterChanged(int componentId, const QString& key,  const QVariant &value)
{
    bool changed = true;
    addComponent(componentId);
    QMap<QString, QVariant>* existParams = getOnbardParametersForComponent(componentId);

    if (existParams->contains(key)) {
        QVariant existValue = existParams->value(key);
        QMap<QString, QVariant>* pendParams = getPendingParametersForComponent(componentId);
        if (pendParams->contains(key)) {
            QVariant pendValue = pendParams->value(key);
            if (existValue == pendValue) {
                changed = false;
            }
        }
    }

    return changed;

}

bool UASParameterDataModel::addPendingIfParameterChanged(int componentId, QString& key,  QVariant &value)

{
    bool changed = checkParameterChanged(componentId,key,value);

    if (changed ) {
        setPendingParameter(componentId,key,value);
    }

    return changed;
}


void UASParameterDataModel::setPendingParameter(int componentId, QString& key,  const QVariant &value)
{
    //ensure we have a placeholder map for this component
    addComponent(componentId);
    QMap<QString, QVariant> *params = getPendingParametersForComponent(componentId);
    params->insert(key,value);
}

void UASParameterDataModel::setOnboardParameter(int componentId, QString& key,  const QVariant& value)
{
    //ensure we have a placeholder map for this component
    addComponent(componentId);
    QMap<QString, QVariant> *params = getOnbardParametersForComponent(componentId);
    params->insert(key,value);
}

void UASParameterDataModel::setOnboardParameterWithType(int componentId, QString& key, QVariant& value)
{

//    switch ((int)onboardParameters.value(componentId)->value(key).type())
    switch ((int)value.type())
    {
    case QVariant::Int:
    {
        QVariant fixedValue(value.toInt());
        onboardParameters.value(componentId)->insert(key, fixedValue);
    }
        break;
    case QVariant::UInt:
    {
        QVariant fixedValue(value.toUInt());
        onboardParameters.value(componentId)->insert(key, fixedValue);
    }
        break;
    case QMetaType::Float:
    {
        QVariant fixedValue(value.toFloat());
        onboardParameters.value(componentId)->insert(key, fixedValue);
    }
        break;
    case QMetaType::QChar:
    {
        QVariant fixedValue(QChar((unsigned char)value.toUInt()));
        onboardParameters.value(componentId)->insert(key, fixedValue);
    }
        break;
    default:
        qCritical() << "ABORTED PARAM UPDATE, NO VALID QVARIANT TYPE";
        return;
    }
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

bool UASParameterDataModel::getOnboardParameterValue(int componentId, const QString& key, QVariant& value) const
{

    if (onboardParameters.contains(componentId)) {
        if (onboardParameters.value(componentId)->contains(key)) {
            value = onboardParameters.value(componentId)->value(key);
            return true;
        }
    }

    return false;
}

void UASParameterDataModel::forgetAllOnboardParameters()
{
    onboardParameters.clear();
}

void UASParameterDataModel::readUpdateParametersFromStream(QTextStream& stream)
{

    bool userWarned = false;

    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (!line.startsWith("#")) {
            QStringList wpParams = line.split("\t");
            int lineMavId = wpParams.at(0).toInt();
            if (wpParams.size() == 5) {
                // Only load parameters for right mav
                if (!userWarned && (uasId != lineMavId)) {
                    //TODO warn the user somehow
                    QString msg = tr("The parameters in the stream have been saved from system %1, but the currently selected system has the ID %2.").arg(lineMavId).arg(uasId);
//                    MainWindow::instance()->showCriticalMessage(
//                                tr("Parameter loading warning"),
//                                tr("The parameters from the file %1 have been saved from system %2, but the currently selected system has the ID %3. If this is unintentional, please click on <READ> to revert to the parameters that are currently onboard").arg(fileName).arg(wpParams.at(0).toInt()).arg(mav->getUASID()));
                    userWarned = true;
                    return;
                }

                bool changed = false;
                int componentId = wpParams.at(1).toInt();
                QString key = wpParams.at(2);
                QString valStr = wpParams.at(3);
                double dblVal = wpParams.at(3).toDouble();
                uint paramType = wpParams.at(4).toUInt();


                if (!onboardParameters.contains(componentId)) {
                    addComponent(componentId);
                    changed = true;
                }
                else {
                    if (fabs((static_cast<float>(onboardParameters.value(componentId)->value(key, dblVal).toDouble())) - (dblVal)) >
                            2.0f * FLT_EPSILON) {
                            changed = true;
                            qDebug() << "Changed" << key << "VAL" << dblVal;
                       }
                }


                if (changed) {
                    switch (paramType)
                    {
                    case MAV_PARAM_TYPE_REAL32:
                        //receivedParameterUpdate(wpParams.at(0).toInt(), componentId, key, valStr.toFloat());
                        setPendingParameter(componentId,key,QVariant(valStr.toFloat()));
                        //setParameter(componentId, key, valStr.toFloat());
                        break;
                    case MAV_PARAM_TYPE_UINT32:
                        //receivedParameterUpdate(wpParams.at(0).toInt(), componentId, key, valStr.toUInt());
                        setPendingParameter(componentId,key, QVariant(valStr.toUInt()));
                        //setParameter(componentId, key, QVariant(valStr.toUInt()));
                        break;
                    case MAV_PARAM_TYPE_INT32:
                        //receivedParameterUpdate(wpParams.at(0).toInt(), componentId, key, valStr.toInt());
                        setPendingParameter(componentId,key,QVariant(valStr.toInt()));
                        //setParameter(componentId, key, QVariant(valStr.toInt()));
                        break;
                    default:
                        qDebug() << "FAILED LOADING PARAM" << key << "UNKNOWN DATA TYPE";
                    }

                    //TODO update display

                }


            }
        }
    }

}

void UASParameterDataModel::writeOnboardParametersToStream(QTextStream& stream, const QString& name)
{
    stream << "# Onboard parameters for system " << name << "\n";
    stream << "#\n";
    stream << "# MAV ID  COMPONENT ID  PARAM NAME  VALUE (FLOAT)\n";

    // Iterate through all components, through all parameters and emit them
    QMap<int, QMap<QString, QVariant>*>::iterator i;
    for (i = onboardParameters.begin(); i != onboardParameters.end(); ++i) {
        // Iterate through the parameters of the component
        int compid = i.key();
        QMap<QString, QVariant>* comp = i.value();
        {
            QMap<QString, QVariant>::iterator j;
            for (j = comp->begin(); j != comp->end(); ++j)
            {
                QString paramValue("%1");
                QString paramType("%1");
                switch ((int)j.value().type())
                {
                case QVariant::Int:
                    paramValue = paramValue.arg(j.value().toInt());
                    paramType = paramType.arg(MAV_PARAM_TYPE_INT32);
                    break;
                case QVariant::UInt:
                    paramValue = paramValue.arg(j.value().toUInt());
                    paramType = paramType.arg(MAV_PARAM_TYPE_UINT32);
                    break;
                case QMetaType::Float:
                    // We store parameters as floats, with only 6 digits of precision guaranteed for decimal string conversion
                    // (see IEEE 754, 32 bit single-precision)
                    paramValue = paramValue.arg((double)j.value().toFloat(), 25, 'g', 6);
                    paramType = paramType.arg(MAV_PARAM_TYPE_REAL32);
                    break;
                default:
                    qCritical() << "ABORTED PARAM WRITE TO FILE, NO VALID QVARIANT TYPE" << j.value();
                    return;
                }
                stream << this->uasId << "\t" << compid << "\t" << j.key() << "\t" << paramValue << "\t" << paramType << "\n";
                stream.flush();
            }
        }
    }
}
