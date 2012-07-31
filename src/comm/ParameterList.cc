/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Implementation of class OpalRT::ParameterList
 *   @author Bryan Godbolt <godbolt@ualberta.ca>
 */

#include "ParameterList.h"
using namespace OpalRT;

ParameterList::ParameterList()
    :params(new QMap<int, QMap<QGCParamID, Parameter> >),
     paramList(new QList<QList<Parameter*> >()),
     reqdServoParams(new QStringList())
{

    QDir settingsDir = QDir(qApp->applicationDirPath());
    if (settingsDir.dirName() == "bin")
        settingsDir.cdUp();
    settingsDir.cd("data");

    // Enforce a list of parameters which are necessary for flight
    reqdServoParams->append("AIL_RIGHT_IN");
    reqdServoParams->append("AIL_CENTER_IN");
    reqdServoParams->append("AIL_LEFT_IN");
    reqdServoParams->append("AIL_RIGHT_OUT");
    reqdServoParams->append("AIL_CENTER_OUT");
    reqdServoParams->append("AIL_LEFT_OUT");
    reqdServoParams->append("ELE_DOWN_IN");
    reqdServoParams->append("ELE_CENTER_IN");
    reqdServoParams->append("ELE_UP_IN");
    reqdServoParams->append("ELE_DOWN_OUT");
    reqdServoParams->append("ELE_CENTER_OUT");
    reqdServoParams->append("ELE_UP_OUT");
    reqdServoParams->append("RUD_LEFT_IN");
    reqdServoParams->append("RUD_CENTER_IN");
    reqdServoParams->append("RUD_RIGHT_IN");

    QString filename(settingsDir.path() + "/ParameterList.xml");
    if ((QFile::exists(filename)) && open(filename)) {

        /* Get a list of the available parameters from opal-rt */
        QMap<QString, unsigned short> *opalParams = new QMap<QString, unsigned short>;
        getParameterList(opalParams);

        /* Iterate over the parameters we want to use in qgc and populate their ids */
        QMap<int, QMap<QGCParamID, Parameter> >::iterator componentIter;
        QMap<QGCParamID, Parameter>::iterator paramIter;
        QString s;
        for (componentIter = params->begin(); componentIter != params->end(); ++componentIter) {
            paramList->append(QList<Parameter*>());
            for (paramIter = (*componentIter).begin(); paramIter != (*componentIter).end(); ++paramIter) {
                paramList->last().append(paramIter.operator ->());
                s = (*paramIter).getSimulinkPath() + (*paramIter).getSimulinkName();
                if (opalParams->contains(s)) {
                    (*paramIter).setOpalID(opalParams->value(s));
                    //                qDebug() << __FILE__ << " Line:" << __LINE__ << ": Successfully added " << s;
                } else {
                    qWarning() << __FILE__ << " Line:" << __LINE__ << ": " << s << " was not found in param list";
                }
            }
        }
        delete opalParams;
    }
}

ParameterList::~ParameterList()
{
    delete params;
    delete paramList;
}

/**
  Get the list of parameters in the simulink model.  This function does not require
  any prior knowlege of the parameters.  It works by first calling OpalGetParameterList to
  get the number of paramters, then allocates the required amount of memory and then gets
  the paramter list using a second call to OpalGetParameterList.
  */
void ParameterList::getParameterList(QMap<QString, unsigned short> *opalParams)
{
    /* inputs */
    unsigned short allocatedParams=0;
    unsigned short allocatedPathLen=0;
    unsigned short allocatedNameLen=0;
    unsigned short allocatedVarLen=0;

    /* outputs */
    unsigned short numParams;
    unsigned short *idParam=NULL;
    unsigned short maxPathLen;
    char **paths=NULL;
    unsigned short maxNameLen;
    char **names=NULL;
    unsigned short maxVarLen;
    char **var=NULL;

    int returnValue;

    returnValue = OpalGetParameterList(allocatedParams, &numParams, idParam,
                                       allocatedPathLen, &maxPathLen, paths,
                                       allocatedNameLen, &maxNameLen, names,
                                       allocatedVarLen, &maxVarLen, var);
    if (returnValue!=E2BIG) {
//        OpalRT::setLastErrorMsg();
        OpalRT::OpalErrorMsg::displayLastErrorMsg();
        return;
    }

    // allocate memory for parameter list

    idParam = new unsigned short[numParams];
    allocatedParams = numParams;

    paths = new char*[numParams];
    for (int i=0; i<numParams; i++)
        paths[i]=new char[maxPathLen];
    allocatedPathLen = maxPathLen;

    names = new char*[numParams];
    for (int i=0; i<numParams; i++)
        names[i] = new char[maxNameLen];
    allocatedNameLen = maxNameLen;

    var = new char*[numParams];
    for (int i=0; i<numParams; i++)
        var[i] = new char[maxVarLen];
    allocatedVarLen = maxVarLen;

    returnValue = OpalGetParameterList(allocatedParams, &numParams, idParam,
                                       allocatedPathLen, &maxPathLen, paths,
                                       allocatedNameLen, &maxNameLen, names,
                                       allocatedVarLen, &maxVarLen, var);

    if (returnValue != EOK) {
//        OpalRT::setLastErrorMsg();
        OpalRT::OpalErrorMsg::displayLastErrorMsg();
        return;
    }

    QString path, name;
    for (int i=0; i<numParams; ++i) {
        path.clear();
        path.append(paths[i]);
        name.clear();
        name.append(names[i]);
        if (path[path.size()-1] == '/')
            opalParams->insert(path+name, idParam[i]);
        else
            opalParams->insert(path+'/'+name, idParam[i]);
    }
//     Dump out the list of parameters
//    QMap<QString, unsigned short>::const_iterator paramPrint;
//    for (paramPrint = opalParams->begin(); paramPrint != opalParams->end(); ++paramPrint)
//        qDebug() << paramPrint.key();


}

int ParameterList::indexOf(const Parameter &p)
{
    // incase p is a copy of the actual parameter we want (i.e., addresses differ)
    Parameter *pPtr = &((*params)[p.getComponentID()][p.getParamID()]);

    QList<QList<Parameter*> >::const_iterator iter;
    int index = -1;
    for (iter = paramList->begin(); iter != paramList->end(); ++iter) {
        if ((index = (*iter).indexOf(pPtr)) != -1)
            return index;
    }
    return index;
}


ParameterList::const_iterator::const_iterator(QList<Parameter> paramList)
{
    this->paramList = QList<Parameter>(paramList);
    index = 0;
}

ParameterList::const_iterator::const_iterator(const const_iterator &other)
{
    paramList = QList<Parameter>(other.paramList);
    index = other.index;
}

ParameterList::const_iterator ParameterList::begin() const
{
    QList<QMap<QGCParamID, Parameter> > compList = params->values();
    QList<Parameter> paramList;
    QList<QMap<QGCParamID, Parameter> >::const_iterator compIter;
    for (compIter = compList.begin(); compIter != compList.end(); ++compIter)
        paramList.append((*compIter).values());
    return const_iterator(paramList);
}

ParameterList::const_iterator ParameterList::end() const
{
    const_iterator iter = begin();
    return iter+=iter.paramList.size();
}

int ParameterList::count()
{
    int count = 0;
    QList<QList<Parameter*> >::const_iterator iter;
    for (iter = paramList->begin(); iter != paramList->end(); ++iter)
        count += (*iter).count();
    return count;
}

/* Functions related to reading the xml config file */

bool ParameterList::open(QString filename)
{
    QFile paramFile(filename);
    if (!paramFile.exists()) {
        /// \todo open dialog box (maybe: that could also go in  comm config window)
        return false;
    }

    if (!paramFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    read(&paramFile);

    paramFile.close();

    return true;
}

bool ParameterList::read(QIODevice *device)
{
    QDomDocument *paramConfig = new QDomDocument();

    QString errorStr;
    int errorLine;
    int errorColumn;

    if (!paramConfig->setContent(device, true, &errorStr, &errorLine,
                                 &errorColumn)) {
        qDebug() << "Error reading XML Parameter File on line: " << errorLine << errorStr;
        return false;
    }

    QDomElement root = paramConfig->documentElement();
    if (root.tagName() != "ParameterList") {
        qDebug() << __FILE__ << __LINE__ << "This is not a parameter list xml file";
        return false;
    }

    QDomElement child = root.firstChildElement("Block");
    while (!child.isNull()) {
        parseBlock(child);
        child = child.nextSiblingElement("Block");
    }

    if (!reqdServoParams->empty()) {
        qDebug() << __FILE__ << __LINE__ << "Missing the following required servo parameters";
        foreach(QString s, *reqdServoParams) {
            qDebug() << s;
        }
    }

    delete paramConfig;
    return true;
}

void ParameterList::parseBlock(const QDomElement &block)
{

    QDomNodeList paramList;
    QDomElement e;
    Parameter *p;
    SubsystemIds id;
    if (block.attribute("name") == "Navigation")
        id = OpalRT::NAV;
    else if (block.attribute("name") == "Controller")
        id = OpalRT::CONTROLLER;
    else if (block.attribute("name") == "ServoOutputs")
        id = OpalRT::SERVO_OUTPUTS;
    else if (block.attribute("name") == "ServoInputs")
        id = OpalRT::SERVO_INPUTS;

    paramList = block.elementsByTagName("Parameter");
    for (int i=0; i < paramList.size(); ++i) {
        e = paramList.item(i).toElement();
        if (e.hasAttribute("SimulinkPath") &&
                e.hasAttribute("SimulinkParameterName") &&
                e.hasAttribute("QGCParamID")) {

            p = new Parameter(e.attribute("SimulinkPath"),
                              e.attribute("SimulinkParameterName"),
                              static_cast<uint8_t>(id),
                              QGCParamID(e.attribute("QGCParamID")));
            (*params)[id].insert(p->getParamID(), *p);
            if (reqdServoParams->contains((QString)p->getParamID()))
                reqdServoParams->removeAt(reqdServoParams->indexOf((QString)p->getParamID()));

            delete p;


        } else {
            qDebug() << __FILE__ << ":" << __LINE__ << ": error in xml doc in block" << block.attribute("name");
        }
    }



}
