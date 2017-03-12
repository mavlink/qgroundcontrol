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

    Fact* GetMixerType(int typeID) {return _mixerTypeMap[typeID];};
    FactMetaData* GetMixerParameterMetaData(int typeID, int parameterID);
    int GetMixerParameterCount(int typeID);

    //    QMap<QString, FactMetaData*> *GetMap() {return &_metaDataMap;};
    void mixerTypesFromHeaders();
    void mixerParameterMetaDataFromHeaders();

private:
    FactMetaData _mixerTypeMetaData;
    QMap<int, Fact*> _mixerTypeMap;
    QMap< int, QMap<int, FactMetaData*> > _mixerParameterMetaDataMap;
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
