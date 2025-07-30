/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PX4ParameterMetaData.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QXmlStreamReader>

QGC_LOGGING_CATEGORY(PX4ParameterMetaDataLog, "qgc.firmwareplugin.px4.px4parametermetadata")

PX4ParameterMetaData::PX4ParameterMetaData(QObject *parent)
    : ParameterMetaData(parent)
{
    qCDebug(PX4ParameterMetaDataLog) << this;
}

PX4ParameterMetaData::~PX4ParameterMetaData()
{
    qCDebug(PX4ParameterMetaDataLog) << this;
}

void PX4ParameterMetaData::_jsonWriteLine(QFile &file, int indent, const QString &line)
{
    while (indent--) {
        (void) file.write("  ");
    }
    (void) file.write(line.toLocal8Bit().constData());
    (void) file.write("\n");
}

void PX4ParameterMetaData::_generateParameterJson()
{
    int indentLevel = 0;
    QFile jsonFile(QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).absoluteFilePath("parameter.json"));
    (void) jsonFile.open(QFile::WriteOnly | QFile::Truncate | QFile::Text);

    _jsonWriteLine(jsonFile, indentLevel++, "{");
    _jsonWriteLine(jsonFile, indentLevel, "\"version\": 1,");
    _jsonWriteLine(jsonFile, indentLevel, "\"uid\": 1,");
    _jsonWriteLine(jsonFile, indentLevel, "\"scope\": \"Firmware\",");
    _jsonWriteLine(jsonFile, indentLevel++, "\"parameters\": [");

    int keyIndex = 0;
    for (const QString& paramName: _params.keys()) {
        const FactMetaData* metaData = _params[paramName];
        _jsonWriteLine(jsonFile, indentLevel++, "{");
        _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"name\": \"%1\",").arg(paramName));
        _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"type\": \"%1\",").arg(metaData->typeToString(metaData->type())));
        if (!metaData->group().isEmpty()) {
            _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"group\": \"%1\",").arg(metaData->group()));
        }
        if (!metaData->category().isEmpty()) {
            _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"category\": \"%1\",").arg(metaData->category()));
        }
        if (!metaData->shortDescription().isEmpty()) {
            QString text = metaData->shortDescription();
            text.replace("\"", "\\\"");
            _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"shortDescription\": \"%1\",").arg(text));
        }
        if (!metaData->longDescription().isEmpty()) {
            QString text = metaData->longDescription();
            text.replace("\"", "\\\"");
            _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"longDescription\": \"%1\",").arg(text));
        }
        if (!metaData->rawUnits().isEmpty()) {
            _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"units\": \"%1\",").arg(metaData->rawUnits()));
        }
        if (metaData->defaultValueAvailable()) {
            _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"defaultValue\": %1,").arg(metaData->rawDefaultValue().toDouble()));
        }
        if (!qIsNaN(metaData->rawIncrement())) {
            _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"increment\": %1,").arg(metaData->rawIncrement()));
        }
        if (metaData->enumValues().count()) {
            _jsonWriteLine(jsonFile, indentLevel++, "\"values\": [");
            for (int i=0; i<metaData->enumValues().count(); i++) {
                _jsonWriteLine(jsonFile, indentLevel++, "{");
                _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"value\": %1,").arg(metaData->enumValues()[i].toDouble()));
                QString text = metaData->enumStrings()[i];
                text.replace("\"", "\\\"");
                _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"description\": \"%1\"").arg(text));
                _jsonWriteLine(jsonFile, --indentLevel, QStringLiteral("}%1").arg(i == metaData->enumValues().count() - 1 ? "" : ","));
            }
            _jsonWriteLine(jsonFile, --indentLevel, "],");
        }
        if (metaData->vehicleRebootRequired()) {
            _jsonWriteLine(jsonFile, indentLevel, "\"rebootRequired\": true,");
        }
        if (metaData->volatileValue()) {
            _jsonWriteLine(jsonFile, indentLevel, "\"volatile\": true,");
        }
        _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"decimalPlaces\": %1,").arg(metaData->decimalPlaces()));
        _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"minValue\": %1,").arg(metaData->rawMin().toDouble()));
        _jsonWriteLine(jsonFile, indentLevel, QStringLiteral("\"maxValue\": %1").arg(metaData->rawMax().toDouble()));
        _jsonWriteLine(jsonFile, --indentLevel, QStringLiteral("}%1").arg(++keyIndex == _params.keys().count() ? "" : ","));
    }

    _jsonWriteLine(jsonFile, --indentLevel, "]");
    _jsonWriteLine(jsonFile, --indentLevel, "}");
}

void PX4ParameterMetaData::loadFromFile(const QString &xmlPath)
{
    qCDebug(PX4ParameterMetaDataLog) << xmlPath;

    QWriteLocker guard(&_lock);

    if (_parsed) {
        qCWarning(PX4ParameterMetaDataLog) << "Internal error: parameter meta data loaded more than once";
        return;
    }
    _parsed = true;

    qCDebug(PX4ParameterMetaDataLog) << "Loading parameter meta data:" << xmlPath;

    QFile xmlFile(xmlPath);

    if (!xmlFile.exists()) {
        qCWarning(PX4ParameterMetaDataLog) << "Internal error: xmlPath mission" << xmlPath;
        return;
    }

    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qCWarning(PX4ParameterMetaDataLog) << "Internal error: Unable to open parameter file:" << xmlPath << xmlFile.errorString();
        return;
    }

    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qCWarning(PX4ParameterMetaDataLog) << "Badly formed XML" << xml.errorString();
        return;
    }

    QString factGroup;
    QString errorString;
    FactMetaData *metaData = nullptr;
    XmlState xmlState = XmlState::None;
    bool badMetaData = true;

    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "parameters") {
                if (xmlState != XmlState::None) {
                    qCWarning(PX4ParameterMetaDataLog) << "Badly formed XML";
                    return;
                }
                xmlState = XmlState::FoundParameters;

            } else if (elementName == "version") {
                if (xmlState != XmlState::FoundParameters) {
                    qCWarning(PX4ParameterMetaDataLog) << "Badly formed XML";
                    return;
                }
                xmlState = XmlState::FoundVersion;

                bool convertOk;
                QString strVersion = xml.readElementText();
                int intVersion = strVersion.toInt(&convertOk);
                if (!convertOk) {
                    qCWarning(PX4ParameterMetaDataLog) << "Badly formed XML";
                    return;
                }
                if (intVersion <= 2) {
                    // We can't read these old files
                    qDebug() << "Parameter version stamp too old, skipping load. Found:" << intVersion << "Want: 3 File:" << xmlPath;
                    return;
                }

            } else if (elementName == "parameter_version_major") {
                // Just skip over for now
            } else if (elementName == "parameter_version_minor") {
                // Just skip over for now

            } else if (elementName == "group") {
                if (xmlState != XmlState::FoundVersion) {
                    // We didn't get a version stamp, assume older version we can't read
                    qDebug() << "Parameter version stamp not found, skipping load" << xmlPath;
                    return;
                }
                xmlState = XmlState::FoundGroup;

                if (!xml.attributes().hasAttribute("name")) {
                    qCWarning(PX4ParameterMetaDataLog) << "Badly formed XML";
                    return;
                }
                factGroup = xml.attributes().value("name").toString();
                qCDebug(PX4ParameterMetaDataLog) << "Found group: " << factGroup;

            } else if (elementName == "parameter") {
                if (xmlState != XmlState::FoundGroup) {
                    qCWarning(PX4ParameterMetaDataLog) << "Badly formed XML";
                    return;
                }
                xmlState = XmlState::FoundParameter;

                if (!xml.attributes().hasAttribute("name") || !xml.attributes().hasAttribute("type")) {
                    qCWarning(PX4ParameterMetaDataLog) << "Badly formed XML";
                    return;
                }

                QString name = xml.attributes().value("name").toString();
                QString type = xml.attributes().value("type").toString();
                QString strDefault =    xml.attributes().value("default").toString();

                QString category = xml.attributes().value("category").toString();
                if (category.isEmpty()) {
                    category = QStringLiteral("Standard");
                }

                bool volatileValue = false;
                bool readOnly = false;
                QString volatileStr = xml.attributes().value("volatile").toString();
                if (volatileStr.compare(QStringLiteral("true")) == 0) {
                    volatileValue = true;
                    readOnly = true;
                }
                if (!volatileValue) {
                    QString readOnlyStr = xml.attributes().value("readonly").toString();
                    if (readOnlyStr.compare(QStringLiteral("true")) == 0) {
                        readOnly = true;
                    }
                }

                qCDebug(PX4ParameterMetaDataLog) << "Found parameter name:" << name << " type:" << type << " default:" << strDefault;

                // Convert type from string to FactMetaData::ValueType_t
                bool unknownType;
                FactMetaData::ValueType_t foundType = FactMetaData::stringToType(type, unknownType);
                if (unknownType) {
                    qCWarning(PX4ParameterMetaDataLog) << "Parameter meta data with bad type:" << type << " name:" << name;
                    return;
                }

                // Now that we know type we can create meta data object and add it to the system
                metaData = new FactMetaData(foundType, this);
                if (_params.contains(name)) {
                    // We can't trust the meta data since we have dups
                    qCWarning(PX4ParameterMetaDataLog) << "Duplicate parameter found:" << name;
                    badMetaData = true;
                    // Reset to default meta data
                    _params[name] = metaData;
                } else {
                    _params[name] = metaData;
                    metaData->setName(name);
                    metaData->setCategory(category);
                    metaData->setGroup(factGroup);
                    metaData->setReadOnly(readOnly);
                    metaData->setVolatileValue(volatileValue);

                    if (xml.attributes().hasAttribute("default") && !strDefault.isEmpty()) {
                        QVariant varDefault;

                        if (metaData->convertAndValidateRaw(strDefault, false, varDefault, errorString)) {
                            metaData->setRawDefaultValue(varDefault);
                        } else {
                            qCWarning(PX4ParameterMetaDataLog) << "Invalid default value, name:" << name << " type:" << type << " default:" << strDefault << " error:" << errorString;
                        }
                    }
                }

            } else {
                // We should be getting meta data now
                if (xmlState != XmlState::FoundParameter) {
                    qCWarning(PX4ParameterMetaDataLog) << "Badly formed XML";
                    return;
                }

                if (!badMetaData) {
                    if (metaData) {
                        if (elementName == "short_desc") {
                            QString text = xml.readElementText();
                            text = text.replace("\n", " ");
                            qCDebug(PX4ParameterMetaDataLog) << "Short description:" << text;
                            metaData->setShortDescription(text);

                        } else if (elementName == "long_desc") {
                            QString text = xml.readElementText();
                            text = text.replace("\n", " ");
                            qCDebug(PX4ParameterMetaDataLog) << "Long description:" << text;
                            metaData->setLongDescription(text);

                        } else if (elementName == "min") {
                            QString text = xml.readElementText();
                            qCDebug(PX4ParameterMetaDataLog) << "Min:" << text;

                            QVariant varMin;
                            if (metaData->convertAndValidateRaw(text, false /* convertOnly */, varMin, errorString)) {
                                metaData->setRawMin(varMin);
                            } else {
                                qCWarning(PX4ParameterMetaDataLog) << "Invalid min value, name:" << metaData->name() << " type:" << metaData->type() << " min:" << text << " error:" << errorString;
                            }

                        } else if (elementName == "max") {
                            QString text = xml.readElementText();
                            qCDebug(PX4ParameterMetaDataLog) << "Max:" << text;

                            QVariant varMax;
                            if (metaData->convertAndValidateRaw(text, false /* convertOnly */, varMax, errorString)) {
                                metaData->setRawMax(varMax);
                            } else {
                                qCWarning(PX4ParameterMetaDataLog) << "Invalid max value, name:" << metaData->name() << " type:" << metaData->type() << " max:" << text << " error:" << errorString;
                            }

                        } else if (elementName == "unit") {
                            QString text = xml.readElementText();
                            qCDebug(PX4ParameterMetaDataLog) << "Unit:" << text;
                            metaData->setRawUnits(text);

                        } else if (elementName == "decimal") {
                            QString text = xml.readElementText();
                            qCDebug(PX4ParameterMetaDataLog) << "Decimal:" << text;

                            bool convertOk;
                            QVariant varDecimals = QVariant(text).toUInt(&convertOk);
                            if (convertOk) {
                                metaData->setDecimalPlaces(varDecimals.toInt());
                            } else {
                                qCWarning(PX4ParameterMetaDataLog) << "Invalid decimals value, name:" << metaData->name() << " type:" << metaData->type() << " decimals:" << text << " error: invalid number";
                            }

                        } else if (elementName == "reboot_required") {
                            QString text = xml.readElementText();
                            qCDebug(PX4ParameterMetaDataLog) << "RebootRequired:" << text;
                            if (text.compare("true", Qt::CaseInsensitive) == 0) {
                                metaData->setVehicleRebootRequired(true);
                            }

                        } else if (elementName == "values") {
                            // doing nothing individual value will follow anyway. May be used for sanity checking.

                        } else if (elementName == "value") {
                            QString enumValueStr = xml.attributes().value("code").toString();
                            QString enumString = xml.readElementText();
                            qCDebug(PX4ParameterMetaDataLog) << "parameter value:"
                                                             << "value desc:" << enumString << "code:" << enumValueStr;

                            QVariant    enumValue;
                            QString     errorString;
                            if (metaData->convertAndValidateRaw(enumValueStr, false /* validate */, enumValue, errorString)) {
                                metaData->addEnumInfo(enumString, enumValue);
                            } else {
                                qCDebug(PX4ParameterMetaDataLog) << "Invalid enum value, name:" << metaData->name()
                                                                 << " type:" << metaData->type() << " value:" << enumValueStr
                                                                 << " error:" << errorString;
                            }
                        } else if (elementName == "increment") {
                            double  increment;
                            bool    ok;
                            QString text = xml.readElementText();
                            increment = text.toDouble(&ok);
                            if (ok) {
                                metaData->setRawIncrement(increment);
                            } else {
                                qCWarning(PX4ParameterMetaDataLog) << "Invalid value for increment, name:" << metaData->name() << " increment:" << text;
                            }

                        } else if (elementName == "boolean") {
                            QVariant    enumValue;
                            metaData->convertAndValidateRaw(1, false /* validate */, enumValue, errorString);
                            metaData->addEnumInfo(tr("Enabled"), enumValue);
                            metaData->convertAndValidateRaw(0, false /* validate */, enumValue, errorString);
                            metaData->addEnumInfo(tr("Disabled"), enumValue);

                        } else if (elementName == "bitmask") {
                            // doing nothing individual bits will follow anyway. May be used for sanity checking.

                        } else if (elementName == "bit") {
                            bool ok = false;
                            unsigned char bit = xml.attributes().value("index").toString().toUInt(&ok);
                            if (ok) {
                                QString bitDescription = xml.readElementText();
                                qCDebug(PX4ParameterMetaDataLog) << "parameter value:"
                                                                 << "index:" << bit << "description:" << bitDescription;

                                if (bit < 31) {
                                    QVariant bitmaskRawValue = 1 << bit;
                                    QVariant bitmaskValue;
                                    QString errorString;
                                    if (metaData->convertAndValidateRaw(bitmaskRawValue, true, bitmaskValue, errorString)) {
                                        metaData->addBitmaskInfo(bitDescription, bitmaskValue);
                                    } else {
                                        qCDebug(PX4ParameterMetaDataLog) << "Invalid bitmask value, name:" << metaData->name()
                                                                         << " type:" << metaData->type() << " value:" << bitmaskValue
                                                                         << " error:" << errorString;
                                    }
                                } else {
                                    qCWarning(PX4ParameterMetaDataLog) << "Invalid value for bitmask, bit:" << bit;
                                }
                            }
                        } else {
                            qCDebug(PX4ParameterMetaDataLog) << "Unknown element in XML: " << elementName;
                        }
                    } else {
                        qCWarning(PX4ParameterMetaDataLog) << "Internal error";
                    }
                }
            }
        } else if (xml.isEndElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "parameter") {
                // Done loading this parameter, validate default value
                if (metaData->defaultValueAvailable()) {
                    QVariant var;

                    if (!metaData->convertAndValidateRaw(metaData->rawDefaultValue(), false /* convertOnly */, var, errorString)) {
                        qCWarning(PX4ParameterMetaDataLog) << "Invalid default value, name:" << metaData->name() << " type:" << metaData->type() << " default:" << metaData->rawDefaultValue() << " error:" << errorString;
                    }
                }

                // Reset for next parameter
                metaData = nullptr;
                badMetaData = false;
                xmlState = XmlState::FoundGroup;
            } else if (elementName == "group") {
                xmlState = XmlState::FoundVersion;
            } else if (elementName == "parameters") {
                xmlState = XmlState::FoundParameters;
            }
        }
        xml.readNext();
    }

#ifdef GENERATE_PARAMETER_JSON
    _generateParameterJson();
#endif
}

FactMetaData *PX4ParameterMetaData::getMetaDataForFact(const QString &paramName, MAV_TYPE vehicleType, FactMetaData::ValueType_t type)
{
    Q_UNUSED(vehicleType)

    QReadLocker guard(&_lock);

    if (!_params.contains(paramName)) {
        qCDebug(PX4ParameterMetaDataLog) << "No metaData for" << paramName << "using generic metadata";
        FactMetaData *metaData = new FactMetaData(type, this);
        _params[paramName] = metaData;
    }

    return _params[paramName];
}

QVersionNumber PX4ParameterMetaData::versionFromFilename(QStringView path)
{
    QFile xmlFile(path.toString());

    if (!xmlFile.exists()) {
        qCWarning(PX4ParameterMetaDataLog) << QStringLiteral("Internal Error: Parameter meta data file '%1'. %2. error: %3").arg(path, QStringLiteral("Does not exist"), QString());
        return QVersionNumber(-1, -1);
    }

    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qCWarning(PX4ParameterMetaDataLog) << QStringLiteral("Internal Error: Parameter meta data file '%1'. %2. error: %3").arg(path, QStringLiteral("Unable to open file"), xmlFile.errorString());
        return QVersionNumber(-1, -1);
    }

    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qCWarning(PX4ParameterMetaDataLog) << QStringLiteral("Internal Error: Parameter meta data file '%1'. %2. error: %3").arg(path, QStringLiteral("Badly formed XML"), xml.errorString());
        return QVersionNumber(-1, -1);
    }

    int majorVersion = -1;
    int minorVersion = -1;
    while (!xml.atEnd() && ((majorVersion == -1) || (minorVersion == -1))) {
        if (xml.isStartElement()) {
            const QString elementName = xml.name().toString();

            if (elementName == "parameter_version_major") {
                bool convertOk;
                const QString strVersion = xml.readElementText();
                majorVersion = strVersion.toInt(&convertOk);
                if (!convertOk) {
                    qCWarning(PX4ParameterMetaDataLog) << QStringLiteral("Internal Error: Parameter meta data file '%1'. %2. error: %3").arg(path, QStringLiteral("Unable to convert parameter_version_major value to int"), QString());
                    return QVersionNumber(-1, -1);
                }
            } else if (elementName == "parameter_version_minor") {
                bool convertOk;
                const QString strVersion = xml.readElementText();
                minorVersion = strVersion.toInt(&convertOk);
                if (!convertOk) {
                    qCWarning(PX4ParameterMetaDataLog) << QStringLiteral("Internal Error: Parameter meta data file '%1'. %2. error: %3").arg(path, QStringLiteral("Unable to convert parameter_version_minor value to int"), QString());
                    return QVersionNumber(-1, -1);
                }
            }
        }

        (void) xml.readNext();
    }

    if (majorVersion == -1) {
        qCWarning(PX4ParameterMetaDataLog) << QStringLiteral("Internal Error: Parameter meta data file '%1'. %2. error: %3").arg(path, QStringLiteral("parameter_version_major is missing"), QString());
    }

    if (minorVersion == -1) {
        qCWarning(PX4ParameterMetaDataLog) << QStringLiteral("Internal Error: Parameter meta data file '%1'. %2. error: %3").arg(path, QStringLiteral("parameter_version_minor tag is missing"), QString());
    }

    return QVersionNumber(majorVersion, minorVersion);
}
