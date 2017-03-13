/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/// @file
///     @author Matthew Coleman <uavflightdirector@gmail.com>

#include "MixerMetaData.h"

#include "mixer_type_id.h"
#include "mixer_types.h"
#include "mixer_io.h"
#include "mixer_parameters.h"

static const char *mixer_parameter_table[][MIXER_PARAMETERS_MIXER_TYPE_COUNT] = MIXER_PARAMETER_TABLE;
static const unsigned mixer_parameter_count[MIXER_PARAMETERS_MIXER_TYPE_COUNT] = MIXER_PARAMETER_COUNTS;
static const unsigned mixer_input_counts[MIXER_PARAMETERS_MIXER_TYPE_COUNT] = MIXER_INPUT_COUNTS;
static const unsigned mixer_output_counts[MIXER_PARAMETERS_MIXER_TYPE_COUNT] = MIXER_OUTPUT_COUNTS;

MixerMetaData::MixerMetaData()
    : _mixerTypeMetaData(FactMetaData::valueTypeInt32, this)
    , _mixerTypeMap()
    , _mixerParameterMetaDataMap()
{
    _mixerTypeMetaData.setName("MIXER_TYPE");
    _mixerTypeMetaData.setGroup("MIXER_TYPES");
    _mixerTypeMetaData.setRawDefaultValue(-1);
    _mixerTypeMetaData.setRawMax(MIXER_TYPES_COUNT);
    _mixerTypeMetaData.setRawMin(0);

    _mixerTypesFromHeaders();
    _mixerParameterMetaDataFromHeaders();
}

MixerMetaData::~MixerMetaData(){
    qDeleteAll(_mixerTypeMap);
    _mixerTypeMap.clear();
}

FactMetaData* MixerMetaData::GetMixerParameterMetaData(int typeID, int parameterID) {
    if(_mixerParameterMetaDataMap.contains(typeID))
        if( _mixerParameterMetaDataMap[typeID].contains(parameterID))
            return _mixerParameterMetaDataMap[typeID][parameterID];
    return nullptr;
}

int MixerMetaData::GetMixerParameterCount(int mixerTypeID){
    if(mixerTypeID >= MIXER_TYPES_COUNT) return 0;
    return mixer_parameter_count[mixerTypeID];
}

int MixerMetaData::GetMixerConnCount(int mixerTypeID, int connType){
    if(connType >= MIXER_TYPES_COUNT) return 0;
    switch(connType){
    case 0:
        return mixer_output_counts[mixerTypeID];
    case 1:
        return mixer_input_counts[mixerTypeID];
    default:
        return 0;
    }
}


void MixerMetaData::_mixerTypesFromHeaders(){
    Fact *mixerType;

    for(int typeID=0; typeID<MIXER_TYPES_COUNT; typeID++){
        QString typeIDStr;
        typeIDStr = QString(MIXER_TYPE_ID[typeID]);
        mixerType = new Fact(-1, MIXER_TYPE_ID[typeID], FactMetaData::valueTypeInt32, this);
        mixerType->setMetaData(&_mixerTypeMetaData);
        mixerType->setRawValue(typeID);
        mixerType->setParent(this);
        _mixerTypeMap[typeID] = mixerType;
    }
}

void MixerMetaData::_mixerParameterMetaDataFromHeaders(){
    FactMetaData *paramMetaData;

    int paramID;
    for(int typeID=0; typeID<MIXER_TYPES_COUNT; typeID++){
        for(paramID=0; paramID<mixer_parameter_count[typeID]; paramID++){
            paramMetaData = new FactMetaData(FactMetaData::valueTypeFloat, this);
            paramMetaData->setName(mixer_parameter_table[typeID][paramID]);
            _mixerParameterMetaDataMap[typeID][paramID] = paramMetaData;
        }
    }
}
