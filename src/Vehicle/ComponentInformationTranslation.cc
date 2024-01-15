/****************************************************************************
 *
 * (c) 2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "ComponentInformationTranslation.h"
#include "JsonHelper.h"
#include "QGCLZMA.h"

#include <QStandardPaths>
#include <QDir>
#include <QJsonArray>
#include <QXmlStreamReader>

QGC_LOGGING_CATEGORY(ComponentInformationTranslationLog, "ComponentInformationTranslationLog")

ComponentInformationTranslation::ComponentInformationTranslation(QObject* parent,
                                                                 QGCCachedFileDownload* cachedFileDownload)
    : QObject(parent), _cachedFileDownload(cachedFileDownload)
{
}

bool ComponentInformationTranslation::downloadAndTranslate(const QString& summaryJsonFile,
                                                           const QString& toTranslateJsonFile, int maxCacheAgeSec)
{
    // Parse summary: find url for current locale
    _toTranslateJsonFile = toTranslateJsonFile;
    QString locale = QLocale::system().name();
    QString url = getUrlFromSummaryJson(summaryJsonFile, locale);
    if (url.isEmpty()) {
        return false;
    }

    // Download file
    connect(_cachedFileDownload, &QGCCachedFileDownload::downloadComplete, this, &ComponentInformationTranslation::onDownloadCompleted);
    if (!_cachedFileDownload->download(url, maxCacheAgeSec)) {
        qCWarning(ComponentInformationTranslationLog) << "Metadata translation download failed";
        disconnect(_cachedFileDownload, &QGCCachedFileDownload::downloadComplete, this, &ComponentInformationTranslation::onDownloadCompleted);
        return false;
    }
    return true;
}

QString ComponentInformationTranslation::getUrlFromSummaryJson(const QString &summaryJsonFile, const QString &locale)
{
    QString         errorString;
    QJsonDocument   jsonDoc;

    if (!JsonHelper::isJsonFile(summaryJsonFile, jsonDoc, errorString)) {
        qCWarning(ComponentInformationTranslationLog) << "Metadata translation summary json file open failed:" << errorString;
        return "";
    }
    QJsonObject jsonObj = jsonDoc.object();

    QJsonObject localeObj = jsonObj[locale].toObject();
    if (localeObj.isEmpty()) {
        qCWarning(ComponentInformationTranslationLog) << "Locale " << locale << " not found in json";
        return "";
    }

    QString url = localeObj["url"].toString();
    if (url.isEmpty()) {
        qCWarning(ComponentInformationTranslationLog) << "Locale " << locale << ": no url set";
    }
    return url;
}

void ComponentInformationTranslation::onDownloadCompleted(QString remoteFile, QString localFile, QString errorMsg)
{
    disconnect(_cachedFileDownload, &QGCCachedFileDownload::downloadComplete, this, &ComponentInformationTranslation::onDownloadCompleted);

    QString tsFileName = localFile;
    bool deleteFile = false;
    if (errorMsg.isEmpty()) {

        // Decompress if needed
        if (localFile.endsWith(".lzma", Qt::CaseInsensitive) || localFile.endsWith(".xz", Qt::CaseInsensitive)) {
            tsFileName = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath("qgc_translation_file_decompressed.ts");
            if (QGCLZMA::inflateLZMAFile(localFile, tsFileName)) {
                deleteFile = true;
            } else {
                errorMsg = "Inflate of compressed json failed, " + remoteFile;
            }
        }
    }

    // Translate json file to new temp file
    QString translatedJsonFilename;
    if (errorMsg.isEmpty()) {
        translatedJsonFilename = translateJsonUsingTS(_toTranslateJsonFile, tsFileName);
        if (translatedJsonFilename.isEmpty()) {
            errorMsg = "Failed to translate json file";
        }
    }

    if (deleteFile) {
        QFile(localFile).remove();
    }

    emit downloadComplete(translatedJsonFilename, errorMsg);
}

QString ComponentInformationTranslation::translateJsonUsingTS(const QString &toTranslateJsonFile, const QString &tsFile)
{
    qCInfo(ComponentInformationTranslationLog) << "Translating" << toTranslateJsonFile << "using" << tsFile;

    // Open JSON and get the 'translation' object
    QString         errorString;
    QJsonDocument   jsonDoc;

    if (!JsonHelper::isJsonFile(toTranslateJsonFile, jsonDoc, errorString)) {
        qCWarning(ComponentInformationTranslationLog) << "Metadata json file to translate open failed:" << errorString;
        return "";
    }
    QJsonObject jsonObj = jsonDoc.object();

    QJsonObject translationObj = jsonObj["translation"].toObject();
    if (translationObj.isEmpty()) {
        qCWarning(ComponentInformationTranslationLog) << "json file does not contain 'translation' object";
        return "";
    }


    // Open and parse TS file into a hash table
    QHash<QString, QString> translations;
    QFile xmlFile(tsFile);
    if (!xmlFile.open(QIODevice::ReadOnly)) {
        qCWarning(ComponentInformationTranslationLog) << "Failed opening TS file";
        return "";
    }

    QXmlStreamReader xml(xmlFile.readAll());
    xmlFile.close();
    if (xml.hasError()) {
        qCWarning(ComponentInformationTranslationLog) << "Badly formed TS (XML)" << xml.errorString();
        return "";
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
        qCWarning(ComponentInformationTranslationLog) << "No translations found in TS file";
        return "";
    }

    // Translate the json document
    jsonDoc.setObject(translate(translationObj, translations, jsonDoc.object()));

    // Write to file
    QString translatedFileName = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath("qgc_translated_metadata.json");

    QFile translatedFile(translatedFileName);
    if (!translatedFile.open(QFile::WriteOnly|QFile::Truncate)) {
        errorString = tr("File open failed: file:error %1 %2").arg(translatedFile.fileName()).arg(translatedFile.errorString());
        return "";
    }
    translatedFile.write(jsonDoc.toJson());
    translatedFile.close();

    qCInfo(ComponentInformationTranslationLog) << "JSON file" << toTranslateJsonFile << "successfully translated to" << translatedFileName;
    return translatedFileName;
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
