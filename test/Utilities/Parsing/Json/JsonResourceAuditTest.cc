#include "JsonResourceAuditTest.h"

#include <QtCore/QDirIterator>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QScopeGuard>

#include "CameraMetaData.h"
#include "FactMetaData.h"
#include "LogManager.h"
#include "MissionCommandList.h"
#include "PowerModulePresetController.h"
#include "QGCSerialPortInfo.h"

void JsonResourceAuditTest::_verifyNoWarnings(const QString& jsonPath, const QString& category)
{
    if (!LogManager::hasCapturedWarning(category)) {
        return;
    }

    QStringList warnings;
    for (const LogEntry& entry : LogManager::capturedMessages(category)) {
        if (entry.level >= LogEntry::Warning) {
            warnings.append(entry.message);
        }
    }
    QFAIL(qPrintable(QStringLiteral("Warnings parsing %1:\n  %2").arg(jsonPath, warnings.join(QStringLiteral("\n  ")))));
}

void JsonResourceAuditTest::_allResourceJsonParsesClean_test()
{
    LogManager::setCaptureEnabled(true);
    const auto captureGuard = qScopeGuard([] { LogManager::setCaptureEnabled(false); });

    QMap<QString, int> filesCheckedPerType;

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
            _verifyNoWarnings(jsonPath, QStringLiteral("FactSystem.FactMetaData"));
        } else if (fileType == QLatin1String(MissionCommandList::qgcFileType)) {
            // Only the generic/generic command list is a base list; all others are overrides
            const bool baseCommandList = jsonPath.endsWith(QStringLiteral("/MavCmdInfoCommon.json"));
            const MissionCommandList commandList(jsonPath, baseCommandList);
            _verifyNoWarnings(jsonPath, QStringLiteral("MissionManager.MissionCommandList"));
        } else if (fileType == QLatin1String("CameraMetaData")) {
            // Loader uses a fixed resource path; there is exactly one file of this type
            qDeleteAll(CameraMetaData::parseCameraMetaData());
            _verifyNoWarnings(jsonPath, QStringLiteral("Camera.CameraMetaData"));
        } else if (fileType == QLatin1String("USBBoardInfo")) {
            QVERIFY2(QGCSerialPortInfo::_loadJsonData(), "USBBoardInfo.json failed to load");
            _verifyNoWarnings(jsonPath, QStringLiteral("Comms.QGCSerialPortInfo"));
        } else if (fileType == QLatin1String("PowerModulePresets")) {
            PowerModulePresetController controller;
            QVERIFY2(!controller.powerModulePresets().isEmpty(), "No power module presets parsed");
            _verifyNoWarnings(jsonPath, QStringLiteral("AutoPilotPlugins.PowerModulePresetController"));
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
}

UT_REGISTER_TEST(JsonResourceAuditTest, TestLabel::Unit, TestLabel::Utilities)
