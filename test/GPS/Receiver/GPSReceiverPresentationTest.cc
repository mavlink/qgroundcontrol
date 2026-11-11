#include "GPSReceiverPresentationTest.h"

#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QStringList>
#include <QtTest/QTest>

#include "GPSReceiverPresentation.h"

void GPSReceiverPresentationTest::_presentation_data()
{
    QTest::addColumn<int>("manufacturer");
    QTest::addColumn<bool>("specific");
    QTest::addColumn<QStringList>("trueKeys");
    QTest::addColumn<QStringList>("falseKeys");

    // Automatic offers Quectel's flash-save consent and notes, for the case that it identifies a Quectel receiver.
    QTest::newRow("automatic") << 0 << false << QStringList{"automatic",         "rtkBase",
                                                            "surveyIn",          "receiverAveraging",
                                                            "surveyAccuracy",    "surveyDuration",
                                                            "fixedBaseAccuracy", "persistentConfiguration",
                                                            "restartOnConnect",  "surveyMaySavePosition"}
                               << QStringList{"specificReceiver", "passive", "observationAccuracyFilter",
                                              "acceptedObservationTime", "reportsSurveyDuration"};
    QTest::newRow("ublox") << 4 << true << QStringList{"specificReceiver",     "rtkBase",        "surveyIn",
                                                       "surveyAccuracy",       "surveyDuration", "fixedBaseAccuracy",
                                                       "reportsSurveyDuration"}
                           << QStringList{"receiverAveraging", "passive", "persistentConfiguration", "restartOnConnect",
                                          "surveyMaySavePosition"};
    QTest::newRow("unicore") << 5 << true << QStringList{"specificReceiver", "rtkBase", "receiverAveraging"}
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
                             << QStringList{"receiverAveraging", "passive", "fixedBaseAccuracy"};
    QTest::newRow("passive") << 7 << true << QStringList{"specificReceiver", "passive"}
                             << QStringList{"rtkBase",
                                            "surveyIn",
                                            "receiverAveraging",
                                            "surveyAccuracy",
                                            "surveyDuration",
                                            "fixedBaseAccuracy",
                                            "persistentConfiguration",
                                            "restartOnConnect",
                                            "surveyMaySavePosition"};
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

UT_REGISTER_TEST_LIGHTWEIGHT(GPSReceiverPresentationTest, TestLabel::Unit)
