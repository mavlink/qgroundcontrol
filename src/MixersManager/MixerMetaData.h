/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef MIXERMETADATA_H
#define MIXERMETADATA_H

#include <QObject>
#include <Fact.h>
#include <FactMetaData.h>
#include <QMap>


class MixerMetaData : public QObject
{
    Q_OBJECT

public:
    MixerMetaData();
    ~MixerMetaData();

    Fact* getMixerType(int typeID) {return _mixerTypeMap[typeID];};
    FactMetaData* getMixerParameterMetaData(int typeID, int parameterID);

    int getMixerParameterCount(int mixerTypeID);
    int getMixerConnCount(int mixerTypeID, int connType);


protected:
    FactMetaData _mixerTypeMetaData;
    QMap<int, Fact*> _mixerTypeMap;

    ///* Map of mixer types containing maps of parameter indexed FactMetaData
    ///  Used as the metadata for parameters of a given mixer type */
    QMap< int, QMap<int, FactMetaData*> > _mixerParameterMetaDataMap;

    void _mixerTypesFromHeaders();
    void _mixerParameterMetaDataFromHeaders();
};


//class MixerParameterMetaData : public FactMetaData
//{
//    Q_OBJECT

//public:
//    MixerTypeMetaData();
//    ~MixerTypeMetaData();

//private:
//};

//class MixerTypeMetaData : public FactMetaData
//{
//    Q_OBJECT

//public:
//    MixerTypeMetaData();
//    ~MixerTypeMetaData();

//private:
//    QVector<MixerParameterMetaData> _mixerParams;
//};

//class MixerConnectionsMetaData : public QObject
//{
//    Q_OBJECT

//public:
//    MixerTypeMetaData();
//    ~MixerTypeMetaData();

//private:
//    QString _mixerName;
//};


//class MixerTypesMetaData : public QObject
//{
//    Q_OBJECT

//public:
//    MixerTypesMetaData();
//    ~MixerTypesMetaData();

//private:
//    QMap<int, MixerTypeMetaData>        _mixerTypeMetaData;
//};


#endif // MIXERMETADATA_H
