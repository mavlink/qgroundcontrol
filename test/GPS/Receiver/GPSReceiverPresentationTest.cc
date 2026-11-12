#include "GPSReceiverPresentationTest.h"

#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QStringList>
#include <QtTest/QTest>

#include "GPSReceiverConfig.h"
#include "GPSReceiverDescriptor.h"
#include "GPSReceiverPresentation.h"

void GPSReceiverPresentationTest::_presentation_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("specific");
    QTest::addColumn<QStringList>("trueKeys");
    QTest::addColumn<QStringList>("falseKeys");

    // Automatic offers Quectel's flash-save consent and notes, for the case that it identifies a Quectel receiver.
    QTest::newRow("automatic") << 0 << false
                               << QStringList{"automatic",
                                              "rtkBase",
                                              "surveyIn",
                                              "receiverAveraging",
                                              "surveyAccuracy",
                                              "surveyDuration",
                                              "fixedBaseAccuracy",
                                              "persistentConfiguration",
                                              "restartOnConnect",
                                              "surveyMaySavePosition",
                                              "correctionConstellations"}
                               << QStringList{"specificReceiver", "passive", "observationAccuracyFilter",
                                              "acceptedObservationTime", "reportsSurveyDuration"};
    QTest::newRow("ublox") << 4 << true
                           << QStringList{"specificReceiver",
                                          "rtkBase",
                                          "surveyIn",
                                          "surveyAccuracy",
                                          "surveyDuration",
                                          "fixedBaseAccuracy",
                                          "reportsSurveyDuration",
                                          "correctionConstellations"}
                           << QStringList{"receiverAveraging", "passive", "persistentConfiguration", "restartOnConnect",
                                          "surveyMaySavePosition"};
    QTest::newRow("unicore") << 5 << true
                             << QStringList{"specificReceiver", "rtkBase", "receiverAveraging",
                                            "correctionConstellations"}
                             << QStringList{"surveyIn",          "passive",
                                            "surveyAccuracy",    "surveyDuration",
                                            "fixedBaseAccuracy", "persistentConfiguration",
                                            "restartOnConnect",  "surveyMaySavePosition"};
    QTest::newRow("quectel") << 6 << true
                             << QStringList{"specificReceiver",
                                            "rtkBase",
                                            "surveyIn",
                                            "surveyAccuracy",
                                            "surveyDuration",
                                            "observationAccuracyFilter",
                                            "acceptedObservationTime",
                                            "reportsSurveyDuration",
                                            "persistentConfiguration",
                                            "restartOnConnect",
                                            "surveyMaySavePosition"}
                             << QStringList{"receiverAveraging", "passive", "fixedBaseAccuracy",
                                            "correctionConstellations"};
    QTest::newRow("passive") << 7 << true << QStringList{"specificReceiver", "passive"}
                             << QStringList{"rtkBase",
                                            "surveyIn",
                                            "receiverAveraging",
                                            "surveyAccuracy",
                                            "surveyDuration",
                                            "fixedBaseAccuracy",
                                            "persistentConfiguration",
                                            "restartOnConnect",
                                            "surveyMaySavePosition",
                                            "correctionConstellations"};
    QTest::newRow("unknown-manufacturer")
        << 999 << false << QStringList{}
        << QStringList{"specificReceiver", "automatic", "rtkBase", "surveyIn", "receiverAveraging", "passive"};
}

void GPSReceiverPresentationTest::_presentation()
{
    QFETCH(int, manufacturer);
    QFETCH(bool, specific);
    QFETCH(QStringList, trueKeys);
    QFETCH(QStringList, falseKeys);
    const GPSReceiverPresentation& presentation = gpsReceiverPresentation(manufacturer);
    const auto value = [&presentation](const QString& key) {
        const auto& metaObject = GPSReceiverPresentation::staticMetaObject;
        const int index = metaObject.indexOfProperty(qPrintable(key));
        return index >= 0 && metaObject.property(index).readOnGadget(&presentation).toBool();
    };
    QCOMPARE(presentation.specificReceiver, specific);
    for (const auto& key : trueKeys) {
        QVERIFY2(value(key), qPrintable(key));
    }
    for (const auto& key : falseKeys) {
        QVERIFY2(GPSReceiverPresentation::staticMetaObject.indexOfProperty(qPrintable(key)) >= 0, qPrintable(key));
        QVERIFY2(!value(key), qPrintable(key));
    }
}

void GPSReceiverPresentationTest::_correctionConstellations()
{
    // The form offers the constellations a family's base can leave out of its corrections, and Automatic those of
    // every family.
    const int withoutQzss =
        (GPSCorrectionConstellation::GLONASS | GPSCorrectionConstellation::Galileo | GPSCorrectionConstellation::BeiDou)
            .toInt();
    for (const GPSType type : {GPSType::UBlox, GPSType::Unicore}) {
        QCOMPARE(gpsReceiverPresentation(gpsReceiverManufacturerForType(type)).correctionConstellations, withoutQzss);
    }
    for (const GPSType type : {GPSType::Trimble, GPSType::Automatic}) {
        QCOMPARE(gpsReceiverPresentation(gpsReceiverManufacturerForType(type)).correctionConstellations,
                 GPS_ALL_CORRECTION_CONSTELLATIONS.toInt());
    }
    for (const GPSType type : {GPSType::Septentrio, GPSType::Femto, GPSType::Quectel}) {
        QCOMPARE(gpsReceiverPresentation(gpsReceiverManufacturerForType(type)).correctionConstellations, 0);
    }
}

UT_REGISTER_TEST_LIGHTWEIGHT(GPSReceiverPresentationTest, TestLabel::Unit)
