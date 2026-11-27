#include "ComponentInformationTranslation.h"

#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QTemporaryFile>
#include <QtCore/QXmlStreamReader>

#include "Compression.h"
#include "JsonParsing.h"
#include "JsonSchemaValidator.h"
#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "VehicleFileFetch.h"
#include "WorkerJob.h"

QGC_LOGGING_CATEGORY(ComponentInformationTranslationLog, "ComponentInformation.ComponentInformationTranslation")

ComponentInformationTranslation::ComponentInformationTranslation(QObject* parent,
                                                                 QGCCachedFileDownload* cachedFileDownload)
    : QObject(parent), _cachedFileDownload(cachedFileDownload)
{
}

bool ComponentInformationTranslation::downloadAndTranslate(const QString& summaryJsonFile,
                                                           const QString& toTranslateJsonFile, int maxCacheAgeSec, const QString& componentName)
{
    // Metadata is authored in English, no translation needed
    const QString locale = QLocale::system().name();
    if (locale.startsWith(QLatin1String("en"))) {
        qCDebug(ComponentInformationTranslationLog) << "Skipping translation for English locale" << locale << "for" << componentName;
        return false;
    }

    cancel();

    // Parse summary: find url for current locale
    _toTranslateJsonFile = toTranslateJsonFile;
    const auto readSummary = [summaryJsonFile, locale, componentName]() {
        return _urlFromSummaryJson(summaryJsonFile, locale, componentName);
    };
    _summaryJob = QGC::runOnWorker(this, readSummary, [this, maxCacheAgeSec](const SummaryEntry& entry) {
        _downloadTranslation(entry, maxCacheAgeSec);
    });
    return true;
}

void ComponentInformationTranslation::cancel()
{
    delete _summaryJob.data();
    delete _translationJob.data();
    if (_downloading) {
        _stopDownload();
        _cachedFileDownload->cancel();
    }
}

void ComponentInformationTranslation::_downloadTranslation(const SummaryEntry& entry, int maxCacheAgeSec)
{
    if (entry.url.isEmpty()) {
        qCWarning(ComponentInformationTranslationLog).noquote() << entry.warning;
        emit downloadComplete(QString(), QString());
        return;
    }

    // The summary comes from the vehicle, so the URL it names is fetched as the fetch of a vehicle's file would
    const QUrl url = VehicleFileFetch::httpUrl(entry.url);
    if (url.isEmpty()) {
        qCWarning(ComponentInformationTranslationLog)
            << "Metadata translation not downloaded: only HTTP(S) is fetched -" << entry.url;
        emit downloadComplete(QString(), QString());
        return;
    }

    // Download file
    _downloading = true;
    connect(_cachedFileDownload, &QGCCachedFileDownload::finished, this, &ComponentInformationTranslation::onDownloadCompleted);
    if (!_cachedFileDownload->download(url.toString(), maxCacheAgeSec)) {
        qCWarning(ComponentInformationTranslationLog) << "Metadata translation download failed";
        _stopDownload();
        emit downloadComplete(QString(), QString());
    }
}

void ComponentInformationTranslation::_stopDownload()
{
    _downloading = false;
    disconnect(_cachedFileDownload, &QGCCachedFileDownload::finished, this,
               &ComponentInformationTranslation::onDownloadCompleted);
}

ComponentInformationTranslation::SummaryEntry ComponentInformationTranslation::_urlFromSummaryJson(
    const QString& summaryJsonFile, const QString& locale, const QString& componentName)
{
    SummaryEntry entry;
    QString         errorString;
    QJsonDocument   jsonDoc;

    if (!JsonParsing::isJsonFile(summaryJsonFile, jsonDoc, errorString, VehicleFileFetch::kMaxSmallFileBytes)) {
        entry.warning = QStringLiteral("Metadata translation summary json file open failed for %1 : %2")
                            .arg(componentName, errorString);
        return entry;
    }
    QJsonObject jsonObj = jsonDoc.object();

    QJsonObject localeObj = jsonObj[locale].toObject();
    if (localeObj.isEmpty()) {
        entry.warning = QStringLiteral("Locale %1 not found in translation json for %2").arg(locale, componentName);
        return entry;
    }

    entry.url = localeObj["url"].toString();
    if (entry.url.isEmpty()) {
        entry.warning = QStringLiteral("Locale %1 has no url in translation json for %2").arg(locale, componentName);
    }
    return entry;
}

void ComponentInformationTranslation::onDownloadCompleted(bool success, const QString &localFile, QString errorMsg, [[maybe_unused]] bool fromCache)
{
    _stopDownload();

    if (!success || !errorMsg.isEmpty()) {
        emit downloadComplete(QString(), errorMsg);
        return;
    }

    // Translate json file to new temp file, where the downloader writes
    qCDebug(ComponentInformationTranslationLog) << "Translating" << _toTranslateJsonFile << "using" << localFile;
    const QString outputDirectory = _cachedFileDownload->fileDownloader()->outputDirectory();
    const auto translate = [toTranslateJsonFile = _toTranslateJsonFile, localFile, outputDirectory]() {
        return translateJsonUsingTS(toTranslateJsonFile, localFile, outputDirectory);
    };
    // The job cannot be stopped. What a dropped one writes goes once it is written, which is at once if it has been.
    const auto removeDropped = [](const Translation& dropped) {
        if (!dropped.fileName.isEmpty()) {
            (void) QFile::remove(dropped.fileName);
        }
    };
    _translationJob = QGC::runOnWorker(
        this, translate,
        [this](const Translation& translation) {
            for (const QString& warning : translation.warnings) {
                qCWarning(ComponentInformationTranslationLog).noquote() << warning;
            }
            if (translation.fileName.isEmpty()) {
                emit downloadComplete(QString(), QStringLiteral("Failed to translate json file"));
                return;
            }
            qCDebug(ComponentInformationTranslationLog)
                << "JSON file" << _toTranslateJsonFile << "successfully translated to" << translation.fileName;
            emit downloadComplete(translation.fileName, QString());
        },
        removeDropped);
}

ComponentInformationTranslation::Translation ComponentInformationTranslation::translateJsonUsingTS(
    const QString& toTranslateJsonFile, const QString& tsFile, const QString& outputDirectory)
{
    Translation result;

    // Open JSON and get the 'translation' object
    QString         errorString;
    QJsonDocument   jsonDoc;

    if (!JsonParsing::isJsonFile(toTranslateJsonFile, jsonDoc, errorString, VehicleFileFetch::kMaxSmallFileBytes)) {
        result.warnings.append(QStringLiteral("Metadata json file to translate open failed: %1").arg(errorString));
        return result;
    }

    QString schemaError;
    if (!JsonSchemaValidator::validate(jsonDoc, QStringLiteral(":/json/component_metadata/translation.schema.json"), schemaError)) {
        result.warnings.append(QStringLiteral("Translation json schema validation failed: %1").arg(schemaError));
    }

    QJsonObject jsonObj = jsonDoc.object();

    QJsonObject translationObj = jsonObj["translation"].toObject();
    if (translationObj.isEmpty()) {
        result.warnings.append(QStringLiteral("json file does not contain 'translation' object"));
        return result;
    }


    // Open and parse TS file into a hash table
    QHash<QString, QString> translations;
    QByteArray tsBytes;
    if (const Compression::Outcome read =
            Compression::decompressFileToMemory(tsFile, tsBytes, VehicleFileFetch::kMaxSmallFileBytes);
        !read) {
        result.warnings.append(QStringLiteral("Failed reading TS file: %1").arg(read.description()));
        return result;
    }

    QXmlStreamReader xml(tsBytes);
    if (xml.hasError()) {
        result.warnings.append(QStringLiteral("Badly formed TS (XML) %1").arg(xml.errorString()));
        return result;
    }

    bool insideTS = false;

    while (!xml.atEnd()) {
        if (xml.isStartElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "TS") {
                insideTS = true;
            } else if (insideTS && elementName == "context") {
                // Read whole <context>
                QString name;
                QString translation;
                bool insideMessage = false;
                while (!xml.atEnd()) {

                    if (xml.isStartElement()) {
                        if (xml.name().toString() == "message") {
                            insideMessage = true;
                        } else if (xml.name().toString() == "name" && !insideMessage) {
                            name = xml.readElementText();
                        } else if (xml.name().toString() == "translation" && insideMessage) {
                            translation = xml.readElementText();
                        }
                    } else if (xml.isEndElement()) {
                        if (xml.name().toString() == "context") {
                            break;
                        } else if (xml.name().toString() == "message") {
                            insideMessage = false;
                        }
                    }

                    xml.readNext();
                }

                if (name != "" && translation != "") {
                    translations[name] = translation;
                }
            }

        } else if (xml.isEndElement()) {
            QString elementName = xml.name().toString();

            if (elementName == "TS") {
                insideTS = false;
            }
        }
        xml.readNext();
    }

    if (translations.isEmpty()) {
        result.warnings.append(QStringLiteral("No translations found in TS file"));
        return result;
    }

    // Translate the json document
    jsonDoc.setObject(translate(translationObj, translations, jsonDoc.object()));

    // A file of its own: translations of different vehicles, or one that was given up, can run at the same time
    QTemporaryFile translatedFile(QDir(outputDirectory).absoluteFilePath("qgc_translated_metadata_XXXXXX.json"));
    translatedFile.setAutoRemove(false);
    if (!translatedFile.open()) {
        return result;
    }
    if (translatedFile.write(jsonDoc.toJson()) < 0) {
        translatedFile.remove();
        return result;
    }
    translatedFile.close();

    result.fileName = translatedFile.fileName();
    return result;
}

QJsonObject ComponentInformationTranslation::translate(const QJsonObject& translationObj,
                                                       const QHash<QString, QString>& translations, QJsonObject doc)
{
    QJsonObject defs = translationObj["$defs"].toObject();
    if (translationObj.contains("items")) {
        doc = translateItems("", defs, translationObj["items"].toObject(), translations, doc);
    }
    if (translationObj.contains("$ref")) {
        doc = translateItems("", defs, defs[getRefName(translationObj["$ref"].toString())].toObject(), translations, doc);
    }
    return doc;
}

QJsonObject ComponentInformationTranslation::translateItems(const QString& prefix, const QJsonObject& defs,
                                                            const QJsonObject& translationObj,
                                                            const QHash<QString, QString>& translations,
                                                            QJsonObject jsonData)
{
    for (auto translationItemIter = translationObj.begin(); translationItemIter != translationObj.end(); ++translationItemIter) {
        QStringList translationKeys;
        if (translationItemIter.key() == "*") {
            translationKeys = jsonData.keys();
        } else {
            translationKeys.append(translationItemIter.key());
        }
        for (const auto& jsonItem : translationKeys) {
            QString nextPrefix = prefix + '/' + jsonItem;
            QJsonObject nextTranslationObj = translationItemIter.value().toObject();
            if (jsonData.contains(jsonItem)) {
                jsonData[jsonItem] = translateTranslationItems(nextPrefix, defs, nextTranslationObj, translations, jsonData[jsonItem]);
            }
        }
    }
    return jsonData;
}

QString ComponentInformationTranslation::getRefName(const QString& ref)
{
    // expected format: '#/$defs/<name>'
    return ref.mid(8);
}

QJsonValue ComponentInformationTranslation::translateTranslationItems(const QString& prefix, const QJsonObject& defs,
                                                                      const QJsonObject& translationObj,
                                                                      const QHash<QString, QString>& translations,
                                                                      QJsonValue jsonData)
{
    if (translationObj.contains("list")) {
        QJsonObject translationList = translationObj["list"].toObject();
        QString key = translationList["key"].toString();
        int idx = 0;
        QJsonArray array = jsonData.toArray();
        for (const auto& listEntry : array) {
            QString value;
            if (!key.isEmpty() && listEntry.toObject().contains(key)) {
                value = listEntry.toObject()[key].toString();
            } else {
                value = QString::number(idx);
            }
            array[idx] = translateTranslationItems(prefix + '/' + value, defs, translationList, translations, listEntry);
            ++idx;
        }
        jsonData = array;
    }
    if (translationObj.contains("translate")) {
        for (const auto& translateName : translationObj["translate"].toArray()) {
            QString translateNameStr = translateName.toString();
            if (jsonData.toObject().contains(translateNameStr)) {
                if (jsonData[translateNameStr].isString()) {
                    auto lookupIter = translations.find(prefix + '/' + translateNameStr);
                    if (lookupIter != translations.end()) {
                        // We need to copy as there's no way to modify nested elements! See https://bugreports.qt.io/browse/QTBUG-25723
                        QJsonObject obj = jsonData.toObject();
                        obj.insert(translateNameStr, lookupIter.value());
                        jsonData = obj;
                    }
                } else if (jsonData[translateNameStr].isArray()) { // List of strings
                    QJsonArray jsonArray = jsonData[translateNameStr].toArray();
                    for (int i=0; i < jsonArray.count(); ++i) {
                        auto lookupIter = translations.find(prefix + '/' + translateNameStr + '/' + QString::number(i));
                        if (lookupIter != translations.end()) {
                            jsonArray.replace(i, lookupIter.value());
                        }
                    }
                    QJsonObject obj = jsonData.toObject();
                    obj[translateNameStr] = jsonArray;
                    jsonData = obj;
                }
            }
        }
    }

    if (translationObj.contains("translate-global")) {
        for (const auto& translateName : translationObj["translate-global"].toArray()) {
            QString translateNameStr = translateName.toString();
            if (jsonData.toObject().contains(translateNameStr)) {
                if (jsonData[translateNameStr].isString()) {
                    auto lookupIter = translations.find("$globals/" + translateNameStr + "/" + jsonData[translateNameStr].toString());
                    if (lookupIter != translations.end()) {
                        QJsonObject obj = jsonData.toObject();
                        obj.insert(translateNameStr, lookupIter.value());
                        jsonData = obj;
                    }
                } else if (jsonData[translateNameStr].isArray()) { // List of strings
                    QJsonArray jsonArray = jsonData[translateNameStr].toArray();
                    for (int i=0; i < jsonArray.count(); ++i) {
                        auto lookupIter = translations.find("$globals/" + translateNameStr + '/' + jsonArray[i].toString());
                        if (lookupIter != translations.end()) {
                            jsonArray.replace(i, lookupIter.value());
                        }
                    }
                    QJsonObject obj = jsonData.toObject();
                    obj[translateNameStr] = jsonArray;
                    jsonData = obj;
                }
            }
        }
    }
    if (translationObj.contains("items")) {
        jsonData = translateItems(prefix, defs, translationObj["items"].toObject(), translations, jsonData.toObject());
    }
    if (translationObj.contains("$ref")) {
        jsonData = translateTranslationItems(prefix, defs, defs[getRefName(translationObj["$ref"].toString())].toObject(), translations, jsonData);
    }
    return jsonData;
}
