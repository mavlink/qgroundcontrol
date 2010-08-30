#include "ParameterList.h"
using namespace OpalRT;

ParameterList::ParameterList()
{
    params = new QMap<int, QMap<QGCParamID, Parameter> >;

    /* Populate the map with parameter names.  There is no elegant way of doing this so all
       parameter paths and names must be known at compile time and defined here.
       Note: This function is written in a way that calls a lot of copy constructors and is
           therefore not particularly efficient.  However since it is only called once memory
           and computation time are sacrificed for code clarity when adding and modifying
           parameters.
           When defining the path, the trailing slash is necessary
       */
    Parameter *p;
    /* Component: Navigation Filter */
    p = new Parameter("avionics_src/sm_ampro/NAV_FILT_INIT/",
                            "Value",
                            OpalRT::NAV_ID,
                            QGCParamID("NAV_FILT_INIT"));
    (*params)[OpalRT::NAV_ID].insert(p->getParamID(), *p);
    delete p;

    p = new Parameter("avionics_src/sm_ampro/Gain/",
                            "Gain",
                            OpalRT::NAV_ID,
                            QGCParamID("TEST_OUTP_GAIN"));
    (*params)[OpalRT::NAV_ID].insert(p->getParamID(), *p);
    delete p;

    /* Component: Log Facility */
    p = new Parameter("avionics_src/sm_ampro/LOG_FILE_ON/",
                            "Value",
                            OpalRT::LOG_ID,
                            QGCParamID("LOG_FILE_ON"));
    (*params)[OpalRT::LOG_ID].insert(p->getParamID(), *p);
    delete p;

    /* Get a list of the available parameters from opal-rt */
    QMap<QString, unsigned short> *opalParams = new QMap<QString, unsigned short>;
    getParameterList(opalParams);

    /* Iterate over the parameters we want to use in qgc and populate their ids */
    QMap<int, QMap<QGCParamID, Parameter> >::iterator componentIter;
    QMap<QGCParamID, Parameter>::iterator paramIter;
    QString s;
    for (componentIter = params->begin(); componentIter != params->end(); ++componentIter)
    {
        for (paramIter = (*componentIter).begin(); paramIter != (*componentIter).end(); ++paramIter)
        {
            s = (*paramIter).getSimulinkPath() + (*paramIter).getSimulinkName();
            if (opalParams->contains(s))
            {
                (*paramIter).setOpalID(opalParams->value(s));
                qDebug() << __FILE__ << " Line:" << __LINE__ << ": Successfully added " << s;
            }
            else
            {
                qDebug() << __FILE__ << " Line:" << __LINE__ << ": " << s << " was not found in param list";
            }
        }
    }
    delete opalParams;

}

ParameterList::~ParameterList()
{
    delete params;
}

ParameterList::const_iterator::const_iterator()
{

}

const_iterator ParameterList::begin()
{

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
    if (returnValue!=E2BIG)
    {
        OpalLink::setLastErrorMsg();
        OpalLink::displayLastErrorMsg();
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

    if (returnValue != EOK)
    {
        OpalLink::setLastErrorMsg();
        OpalLink::displayLastErrorMsg();
        return;
    }

    QString path, name;
    for (int i=0; i<numParams; ++i)
    {
        path.clear();
        path.append(paths[i]);
        name.clear();
        name.append(names[i]);
        if (path[path.size()-1] == '/')
            opalParams->insert(path+name, idParam[i]);
        else
            opalParams->insert(path+'/'+name, idParam[i]);
    }
    // Dump out the list of parameters
//    QMap<QString, unsigned short>::const_iterator paramPrint;
//    for (paramPrint = opalParams->begin(); paramPrint != opalParams->end(); ++paramPrint)
//        qDebug() << paramPrint.key();


}
