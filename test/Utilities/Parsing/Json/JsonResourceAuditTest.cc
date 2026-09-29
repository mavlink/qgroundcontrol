#include "JsonResourceAuditTest.h"

#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QScopeGuard>
#include <QtCore/QTranslator>

#include "CameraMetaData.h"
#include "FactMetaData.h"
#include "JsonParsing.h"
#include "LogManager.h"
#include "MissionCommandList.h"
#include "PowerModulePresetController.h"
#ifndef QGC_NO_SERIAL_LINK
#include "QGCSerialPortInfo.h"
#endif

void JsonResourceAuditTest::_collectWarnings(const QString& jsonPath, const QString& category, QStringList& failures)
{
    QStringList warnings;
    for (const LogEntry& entry : LogManager::capturedMessages(category)) {
        if (entry.level >= LogEntry::Warning) {
            warnings.append(entry.message);
        }
    }
    if (!warnings.isEmpty()) {
        failures.append(QStringLiteral("%1:\n    %2").arg(jsonPath, warnings.join(QStringLiteral("\n    "))));
    }
}

void JsonResourceAuditTest::_allResourceJsonParsesClean_test_data()
{
    QTest::addColumn<QString>("translationFile");

    QTest::newRow("untranslated") << QString();

    // A translation can break a file that parses clean untranslated, e.g. by dropping an enum string
    QDirIterator it(QStringLiteral(":/i18n"), {QStringLiteral("qgc_json_*.qm")}, QDir::Files);
    while (it.hasNext()) {
        const QString translationFile = it.next();
        QTest::newRow(qPrintable(QFileInfo(translationFile).completeBaseName())) << translationFile;
    }
}

void JsonResourceAuditTest::_allResourceJsonParsesClean_test()
{
    QFETCH(QString, translationFile);

    // An empty path clears the translator, so no row depends on the host locale; the startup catalog is restored after
    QTranslator* const translator = JsonParsing::translator();
    const QString startupCatalog = translator->filePath();
    const auto translatorGuard = qScopeGuard([translator, startupCatalog] { (void) translator->load(startupCatalog); });
    const bool translationLoaded = translator->load(translationFile);
    QVERIFY2(translationLoaded || translationFile.isEmpty(), qPrintable(translationFile));

    LogManager::setCaptureEnabled(true);
    const auto captureGuard = qScopeGuard([] { LogManager::setCaptureEnabled(false); });

    QMap<QString, int> filesCheckedPerType;
    QStringList failures;

    QDirIterator it(QStringLiteral(":/"), {QStringLiteral("*.json")}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString jsonPath = it.next();

        // Unit test fixtures may be deliberately malformed
        if (jsonPath.startsWith(QStringLiteral(":/unittest/"))) {
            continue;
        }

        QFile file(jsonPath);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(jsonPath));
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
        QVERIFY2(parseError.error == QJsonParseError::NoError,
                 qPrintable(QStringLiteral("%1: %2 at offset %3")
                                .arg(jsonPath, parseError.errorString())
                                .arg(parseError.offset)));
        // Non-object roots cannot carry a fileType and are out of audit scope
        if (!doc.isObject()) {
            continue;
        }
        const QString fileType = doc.object().value(QStringLiteral("fileType")).toString();
        if (fileType.isEmpty()) {
            continue;
        }

        LogManager::clearCapturedMessages();

        if (fileType == QLatin1String(FactMetaData::qgcFileType)) {
            QObject parent;
            // An empty map is legal (e.g. Units.SettingsGroup.json declares no facts); actual
            // parse failures surface as warnings, checked below
            (void) FactMetaData::createMapFromJsonFile(jsonPath, &parent);
            _collectWarnings(jsonPath, QStringLiteral("FactSystem.FactMetaData"), failures);
        } else if (fileType == QLatin1String(MissionCommandList::qgcFileType)) {
            // Only the generic/generic command list is a base list; all others are overrides
            const bool baseCommandList = jsonPath.endsWith(QStringLiteral("/MavCmdInfoCommon.json"));
            const MissionCommandList commandList(jsonPath, baseCommandList);
            _collectWarnings(jsonPath, QStringLiteral("MissionManager.MissionCommandList"), failures);
        } else if (fileType == QLatin1String("CameraMetaData")) {
            // Loader uses a fixed resource path; there is exactly one file of this type
            qDeleteAll(CameraMetaData::parseCameraMetaData());
            _collectWarnings(jsonPath, QStringLiteral("Camera.CameraMetaData"), failures);
        } else if (fileType == QLatin1String("USBBoardInfo")) {
#ifndef QGC_NO_SERIAL_LINK
            QVERIFY2(QGCSerialPortInfo::_loadJsonData(), "USBBoardInfo.json failed to load");
            _collectWarnings(jsonPath, QStringLiteral("Comms.QGCSerialPortInfo"), failures);
#endif
        } else if (fileType == QLatin1String("PowerModulePresets")) {
            PowerModulePresetController controller;
            QVERIFY2(!controller.powerModulePresets().isEmpty(), "No power module presets parsed");
            _collectWarnings(jsonPath, QStringLiteral("AutoPilotPlugins.PowerModulePresetController"), failures);
        } else {
            // SettingsUI / SettingsPages / VehicleConfig are consumed by the Python QML
            // generators at configure time, which validate them at build time
            continue;
        }

        filesCheckedPerType[fileType]++;
    }

    QVERIFY2(filesCheckedPerType.value(QLatin1String(FactMetaData::qgcFileType)) > 0,
             "No FactMetaData JSON found in resources - resource iteration is broken");
    QVERIFY2(filesCheckedPerType.value(QLatin1String(MissionCommandList::qgcFileType)) > 0,
             "No MavCmdInfo JSON found in resources - resource iteration is broken");
    QVERIFY2(
        failures.isEmpty(),
        qPrintable(QStringLiteral("Warnings parsing resource JSON:\n  %1").arg(failures.join(QStringLiteral("\n  ")))));
}

UT_REGISTER_TEST(JsonResourceAuditTest, TestLabel::Unit, TestLabel::Utilities)
