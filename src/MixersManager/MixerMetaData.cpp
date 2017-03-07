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
    : _mixerTypeMetaData(FactMetaData::valueTypeString, this)
    , _mixerTypeMap()
    , _mixerParameterMetaDataMap()
{
    _mixerTypeMetaData.setName("MIXER_TYPE");
    _mixerTypeMetaData.setGroup("MIXER_META_DATA");
    _mixerTypeMetaData.setRawDefaultValue("MIXER_TYPE_UNKNOWN");

    mixerTypesFromHeaders();
    mixerParameterMetaDataFromHeaders();
}

MixerMetaData::~MixerMetaData(){
    qDeleteAll(_mixerTypeMap);
    _mixerTypeMap.clear();

//    qDeleteAll(_mixerParameterMetaDataMap);
//    _mixerParameterMetaDataMap.clear();
}

FactMetaData* MixerMetaData::GetMixerParameterMetaData(int typeID, int parameterID) {
    if(_mixerParameterMetaDataMap.contains(typeID))
        if( _mixerParameterMetaDataMap[typeID].contains(parameterID))
            return _mixerParameterMetaDataMap[typeID][parameterID];
    return nullptr;
};

void MixerMetaData::mixerTypesFromHeaders(){
    Fact *mixerType;

    for(int typeID=0; typeID<MIXER_TYPES_COUNT; typeID++){
        mixerType = new Fact(-1, MIXER_TYPE_ID[typeID], FactMetaData::valueTypeString, this);
        mixerType->setMetaData(&_mixerTypeMetaData);
        _mixerTypeMap[typeID] = mixerType;
    }
}

void MixerMetaData::mixerParameterMetaDataFromHeaders(){
    FactMetaData *paramMetaData;

    int paramID;
    for(int typeID=0; typeID<MIXER_TYPES_COUNT; typeID++){
//        qDebug("Mixer Type:%u parameter count:%u", typeID,  mixer_parameter_count[typeID]);
        for(paramID=0; paramID<mixer_parameter_count[typeID]; paramID++){
            paramMetaData = new FactMetaData(FactMetaData::valueTypeFloat, this);
            paramMetaData->setName(mixer_parameter_table[typeID][paramID]);
            _mixerParameterMetaDataMap[typeID][paramID] = paramMetaData;
        }
    }
}

