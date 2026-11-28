#pragma once

#include <any>
#include <functional>

#include <QtCore/QJsonDocument>
#include <QtCore/QObject>
#include <QtCore/QString>

#include "MAVLinkEnums.h"

class FactMetaData;
class Vehicle;
class FirmwarePlugin;
class CompInfoGeneral;

/// \brief Base class for all CompInfo types
///
class CompInfo : public QObject
{
    Q_OBJECT

public:
    CompInfo(COMP_METADATA_TYPE type_, uint8_t compId_, Vehicle* vehicle_, QObject* parent = nullptr);

    const QString& uriMetaData() const { return _uris.uriMetaData; }
    const QString& uriMetaDataFallback() const { return _uris.uriMetaDataFallback; }
    const QString& uriTranslation() const { return _uris.uriTranslation; }

    uint32_t crcMetaData() const { return _uris.crcMetaData; }
    uint32_t crcMetaDataFallback() const { return _uris.crcMetaDataFallback; }
    bool crcMetaDataValid() const { return _uris.crcMetaDataValid; }
    bool crcMetaDataFallbackValid() const { return _uris.crcMetaDataFallbackValid; }

    void setUriMetaData(const QString& uri, uint32_t crc);

    /// What parseJsonFile() made of a metadata file
    struct ParsedJson
    {
        QJsonDocument document;  ///< Null if the file could not be read or parsed
        QString readError;       ///< Why the file could not be read or parsed
        QString schemaError;     ///< Why the document does not match its schema, which does not stop its use
        std::any data;           ///< What the fileParser() of a type that parses the file itself made of it
        bool read = false;       ///< Whether the file could be read and parsed, so that it may be cached
    };

    /// Reads and parses a metadata file, decompressed if it is compressed and held to
    /// VehicleFileFetch::kMaxSmallFileBytes, and checks it against the JSON schema @p schemaResourcePath. It uses
    /// nothing but its arguments, so that it can run on a worker thread: the file comes from the vehicle, or from where
    /// the vehicle points to, and may be large.
    static ParsedJson parseJsonFile(const QString& fileName, const QString& schemaResourcePath);

    /// JSON schema of this type of metadata, for parseJsonFile()
    virtual QString jsonSchema() const = 0;

    using FileParser = std::function<ParsedJson(const QString& fileName)>;

    /// Reads and parses a metadata file of this type, on a worker thread, so it holds copies of what it uses and
    /// never this object. parseJsonFile() with jsonSchema() unless a type parses the file itself.
    virtual FileParser fileParser() const;

    /// Takes the metadata in @p fileName. Called on the thread of this object, with what fileParser() made of the
    /// file, or with nothing if @p fileName is empty, which means there is no metadata.
    virtual void setJson(const QString& fileName, const ParsedJson& json) = 0;

    bool available() const { return !_uris.uriMetaData.isEmpty(); }

    const COMP_METADATA_TYPE  type;
    Vehicle* const      vehicle                = nullptr;
    const uint8_t       compId                 = MAV_COMP_ID_ALL;

private:
    friend class CompInfoGeneral;

    struct Uris {
        bool                crcMetaDataValid            = false;
        bool                crcMetaDataFallbackValid    = false;

        uint32_t            crcMetaData            = 0;
        uint32_t            crcMetaDataFallback    = 0;

        QString             uriMetaData;
        QString             uriMetaDataFallback;
        QString             uriTranslation;
        QString             uriTranslationFallback;
    };

    Uris _uris;
};
