#include "UASParameterDataModel.h"

#include <float.h>

#include <QDebug>
#include <QStringList>
#include <QVariant>

#include "QGCMAVLink.h"

UASParameterDataModel::UASParameterDataModel(QObject *parent) :
    QObject(parent)
{
    onboardParameters.clear();
    pendingParameters.clear();

}



bool UASParameterDataModel::checkParameterChanged(int compId, const QString& key,  const QVariant& value)
{
    bool changed = true;
    //ensure we have this component in our onboard and pending lists already
    addComponent(compId);
    QMap<QString, QVariant>* existParams = getOnbardParametersForComponent(compId);

    if (existParams->contains(key)) {
        QVariant existValue = existParams->value(key);
        if (existValue == value) {
            changed = false;
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

void UASParameterDataModel::addComponent(int compId)
{
    if (!onboardParameters.contains(compId)) {
        onboardParameters.insert(compId, new QMap<QString, QVariant>());
    }
    if (!pendingParameters.contains(compId)) {
        pendingParameters.insert(compId, new QMap<QString, QVariant>());
    }
}


void UASParameterDataModel::handleParameterUpdate(int compId, QString& key, QVariant& value)
{
    //verify that the value requested by the user matches the set value
    //if it doesn't match, leave the pending parameter in the pending list!
    if (pendingParameters.contains(compId)) {
        QMap<QString , QVariant> *pendingParams = pendingParameters.value(compId);
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

    setOnboardParameter(compId,key,value);

    emit parameterUpdated(compId,key,value);

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

void UASParameterDataModel::readUpdateParametersFromStream( QTextStream& stream)
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

void UASParameterDataModel::writeOnboardParametersToStream( QTextStream &stream, const QString& name)
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


void UASParameterDataModel::loadParamMetaInfoFromStream(QTextStream& stream)
{

    // First line is header
    // there might be more lines, but the first
    // line is assumed to be at least header
    QString header = stream.readLine();

    // Ignore top-level comment lines
    while (header.startsWith('#') || header.startsWith('/')
           || header.startsWith('=') || header.startsWith('^'))
    {
        header = stream.readLine();
    }

    bool charRead = false;
    QString separator = "";
    QList<QChar> sepCandidates;
    sepCandidates << '\t';
    sepCandidates << ',';
    sepCandidates << ';';
    //sepCandidates << ' ';
    sepCandidates << '~';
    sepCandidates << '|';

    // Iterate until separator is found
    // or full header is parsed
    for (int i = 0; i < header.length(); i++)
    {
        if (sepCandidates.contains(header.at(i)))
        {
            // Separator found
            if (charRead)
            {
                separator += header[i];
            }
        }
        else
        {
            // Char found
            charRead = true;
            // If the separator is not empty, this char
            // has been read after a separator, so detection
            // is now complete
            if (separator != "") break;
        }
    }

    bool stripFirstSeparator = false;
    bool stripLastSeparator = false;

    // Figure out if the lines start with the separator (e.g. wiki syntax)
    if (header.startsWith(separator)) stripFirstSeparator = true;

    // Figure out if the lines end with the separator (e.g. wiki syntax)
    if (header.endsWith(separator)) stripLastSeparator = true;

    QString out = separator;
    out.replace("\t", "<tab>");
    //qDebug() << " Separator: \"" << out << "\"";
    //qDebug() << "READING CSV:" << header;


    // Read data
    while (!stream.atEnd())
    {
        QString line = stream.readLine();

        //qDebug() << "LINE PRE-STRIP" << line;

        // Strip separtors if necessary
        if (stripFirstSeparator) line.remove(0, separator.length());
        if (stripLastSeparator) line.remove(line.length()-separator.length(), line.length()-1);

        //qDebug() << "LINE POST-STRIP" << line;

        // Keep empty parts here - we still have to act on them
        QStringList parts = line.split(separator, QString::KeepEmptyParts);

        // Each line is:
        // variable name, Min, Max, Default, Multiplier, Enabled (0 = no, 1 = yes), Comment


        // Fill in min, max and default values
        if (parts.count() > 1)
        {
            // min
            paramMin.insert(parts.at(0).trimmed(), parts.at(1).toDouble());
        }
        if (parts.count() > 2)
        {
            // max
            paramMax.insert(parts.at(0).trimmed(), parts.at(2).toDouble());
        }
        if (parts.count() > 3)
        {
            // default
            paramDefault.insert(parts.at(0).trimmed(), parts.at(3).toDouble());
        }
        // IGNORING 4 and 5 for now
        if (parts.count() > 6)
        {
            // tooltip
            paramDescriptions.insert(parts.at(0).trimmed(), parts.at(6).trimmed());
            qDebug() << "PARAM META:" << parts.at(0).trimmed();
        }
    }
}

 void UASParameterDataModel::setParamDescriptions(const QMap<QString,QString>& paramInfo)
{
    if (paramInfo.isEmpty()) {
        qDebug() << __FILE__ << ":" << __LINE__ << "setParamDescriptions with empty";
    }

    paramDescriptions = paramInfo;
}

bool UASParameterDataModel::isValueGreaterThanParamMax(const QString& paramName, double dblVal)
{
    if (paramMax.contains(paramName)) {
        if (dblVal > paramMax.value(paramName))
            return true;
    }

    return false;
}

bool UASParameterDataModel::isValueLessThanParamMin(const QString& paramName, double dblVal)
{
     if (paramMin.contains(paramName)) {
         if (dblVal < paramMin.value(paramName))
             return true;
     }

     return false;
 }

