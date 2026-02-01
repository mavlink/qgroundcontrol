#pragma once


#include "MAVLinkLib.h"
#include "FactMetaData.h"

#include <QtCore/QObject>


//#define GENERATE_PARAMETER_JSON

/// Loads and holds parameter fact meta data for PX4 stack
class PX4ParameterMetaData : public QObject
{
    Q_OBJECT

public:
    PX4ParameterMetaData(QObject* parent = nullptr);

    void            loadParameterFactMetaDataFile   (const QString& metaDataFile);
    FactMetaData*   getMetaDataForFact              (const QString& name, MAV_TYPE vehicleType, FactMetaData::ValueType_t type);

    static void getParameterMetaDataVersionInfo(const QString& metaDataFile, int& majorVersion, int& minorVersion);

private:
    enum {
        XmlStateNone,
        XmlStateFoundParameters,
        XmlStateFoundVersion,
        XmlStateFoundGroup,
        XmlStateFoundParameter,
        XmlStateDone
    };

    QVariant _stringToTypedVariant(const QString& string, FactMetaData::ValueType_t type, bool* convertOk);
    static void _outputFileWarning(const QString& metaDataFile, const QString& error1, const QString& error2);

#ifdef GENERATE_PARAMETER_JSON
    void _generateParameterJson();
#endif

    bool                                _parameterMetaDataLoaded        = false;    ///< true: parameter meta data already loaded
    FactMetaData::NameToMetaDataMap_t   _mapParameterName2FactMetaData;             ///< Maps from a parameter name to FactMetaData

    static constexpr const char* kInvalidConverstion = "Internal Error: No support for string parameters";

};
