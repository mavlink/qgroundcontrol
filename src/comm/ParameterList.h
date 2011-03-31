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

#ifndef PARAMETERLIST_H
#define PARAMETERLIST_H

#include <QMap>
#include <QVector>
#include <QIODevice>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QtXml>
#include <QStringList>

#include "mavlink_types.h"
#include "QGCParamID.h"
#include "Parameter.h"
#include "OpalRT.h"

namespace OpalRT
{
class ParameterList
{
public:

    class const_iterator
    {
        friend class ParameterList;

    public:
        inline const_iterator() {}
        const_iterator(const const_iterator& other);

        const_iterator& operator+=(int i) {
            index += i;
            return *this;
        }
        bool operator<(const const_iterator& other) const {
            return (this->paramList == other.paramList)
                   &&(this->index<other.index);
        }
        bool operator==(const const_iterator& other) const {
            return (this->paramList == other.paramList)
                   &&(this->index==other.index);
        }
        bool operator!=(const const_iterator& other) const {
            return !((*this) == other);
        }
        const Parameter& operator*() const {
            return paramList[index];
        }
        const Parameter* operator->() const {
            return &paramList[index];
        }

        const_iterator& operator++() {
            ++index;
            return *this;
        }
    private:
        const_iterator(QList<Parameter>);
        QList<Parameter> paramList;
        int index;
    };


    ParameterList();
    ~ParameterList();

    /** Count the number of parameters in the list.
      \return Total number of parameters
      */
    int count();

    /** Find p in the list and return its index.
      \note In order to use this index to look up p, the component is also needed.
      \return the index of p or -1 if p is not found
      \example
      \code
      int compid = OpalRT::CONTROLLER_ID;
      Parameter p("simulinkpath", "simulinkparamname", compid, QGCParamID("PID_GAIN"));
      ParameterList pList;
      if ((int index=pList.indexOf(p)) != -1)
         qDebug() << "PID_GAIN is at index " << index;
      \endcode
      */
    int indexOf(const Parameter& p);
    bool contains(int compid, QGCParamID paramid) const {
        return (*params)[compid].contains(paramid);
    }

    /// Get a parameter from the list
    const Parameter getParameter(int compid, QGCParamID paramid) const {
        return (*params)[compid][paramid];
    }
    Parameter& getParameter(int compid, QGCParamID paramid) {
        return (*params)[compid][paramid];
    }
    const Parameter getParameter(int compid, int index) const {
        return *((*paramList)[compid][index]);
    }

    /** Convenient syntax for calling OpalRT::Parameter::getParameter() */
    Parameter& operator()(int compid, QGCParamID paramid) {
        return getParameter(compid, paramid);
    }
    Parameter& operator()(uint8_t compid, QGCParamID paramid) {
        return getParameter(static_cast<int>(compid), paramid);
    }

    const_iterator begin() const;
    const_iterator end() const;

protected:
    /** Store the parameters mapped by componentid, and paramid.
       \code
       // Look up a parameter
       int compid = 1;
       QGCParamID paramid("PID_GAIN");
       Parameter p = params[compid][paramid];
       \endcode
       */
    QMap<int, QMap<QGCParamID, Parameter> > *params;
    /**
      Store pointers to the parameters to allow fast lookup by index.
      This variable may be changed to const pointers to ensure all changes
      are made through the map container.
      */
    QList<QList<Parameter*> > *paramList;
    /**
      List of parameters which are necessary to control the servos.
      */
    QStringList *reqdServoParams;
    /**
      Get the list of available parameters from Opal-RT.
      \param[out] opalParams Map of parameter paths/names to ids which are valid in Opal-RT
      */
    void getParameterList(QMap<QString, unsigned short>* opalParams);

    /**
      Open a file for reading in the xml config data
      */
    bool open(QString filename=QString());
    /**
      Attempt to read XML configuration data from device
      \param[in] the device to read the xml data from
      \return true if the configuration was read successfully, false otherwise
      */
    bool read(QIODevice *device);

    void parseBlock(const QDomElement &block);
};
}
#endif // PARAMETERLIST_H
