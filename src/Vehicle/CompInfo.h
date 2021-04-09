/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCMAVLink.h"
#include "QGCLoggingCategory.h"
#include "FactMetaData.h"

#include <QObject>

class FactMetaData;
class Vehicle;
class FirmwarePlugin;
class CompInfoGeneral;

/// Base class for all CompInfo types
class CompInfo : public QObject
{
    Q_OBJECT

public:
    CompInfo(COMP_METADATA_TYPE type, uint8_t compId, Vehicle* vehicle, QObject* parent = nullptr);

    const QString& uriMetaData() const { return _uris.uriMetaData; }
    const QString& uriMetaDataFallback() const { return _uris.uriMetaDataFallback; }
    const QString& uriTranslation() const { return _uris.uriTranslation; }

    uint32_t crcMetaData() const { return _uris.crcMetaData; }
    uint32_t crcMetaDataFallback() const { return _uris.crcMetaDataFallback; }
    bool crcMetaDataValid() const { return _uris.crcMetaDataValid; }
    bool crcMetaDataFallbackValid() const { return _uris.crcMetaDataFallbackValid; }
    uint32_t crcTranslation() const { return _uris.crcTranslation; }
    bool crcTranslationValid() const { return _uris.crcTranslationValid; }

    void setUriMetaData(const QString& uri, uint32_t crc);

    virtual void setJson(const QString& metaDataJsonFileName, const QString& translationJsonFileName) = 0;

    bool available() const { return !_uris.uriMetaData.isEmpty(); }

    const COMP_METADATA_TYPE  type;
    Vehicle* const      vehicle                = nullptr;
    const uint8_t       compId                 = MAV_COMP_ID_ALL;

private:
    friend class CompInfoGeneral;

    struct Uris {
        bool                crcMetaDataValid            = false;
        bool                crcMetaDataFallbackValid    = false;
        bool                crcTranslationValid         = false;
        bool                crcTranslationFallbackValid = false;

        uint32_t            crcMetaData            = 0;
        uint32_t            crcMetaDataFallback    = 0;
        uint32_t            crcTranslation         = 0;
        uint32_t            crcTranslationFallback = 0;

        QString             uriMetaData;
        QString             uriMetaDataFallback;
        QString             uriTranslation;
        QString             uriTranslationFallback;
    };

    Uris _uris;
};
