#include "JsonSchemaValidatorTest.h"

#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include "JsonSchemaValidator.h"

void JsonSchemaValidatorTest::_validActuatorsExampleValidates_test()
{
    QFile exampleFile(QStringLiteral(":/unittest/actuators.example.json"));
    QVERIFY(exampleFile.open(QIODevice::ReadOnly));

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(exampleFile.readAll(), &parseError);
    QCOMPARE(parseError.error, QJsonParseError::NoError);

    QString errorString;
    const bool ok = JsonSchemaValidator::validate(
        doc, QStringLiteral(":/json/component_metadata/actuators.schema.json"), errorString);
    QVERIFY2(ok, qPrintable(errorString));
    QVERIFY(errorString.isEmpty());
}

void JsonSchemaValidatorTest::_malformedObjectFailsValidation_test()
{
    QJsonObject malformed;
    malformed.insert(QStringLiteral("version"), QStringLiteral("not-an-integer"));
    malformed.insert(QStringLiteral("parameters"), 42);
    const QJsonDocument doc(malformed);

    QString errorString;
    const bool ok = JsonSchemaValidator::validate(
        doc, QStringLiteral(":/json/component_metadata/parameter.schema.json"), errorString);
    QVERIFY(!ok);
    QVERIFY(!errorString.isEmpty());
}

void JsonSchemaValidatorTest::_missingSchemaResourceFails_test()
{
    const QJsonDocument doc(QJsonObject{});

    QString errorString;
    const bool ok = JsonSchemaValidator::validate(
        doc, QStringLiteral(":/json/component_metadata/does_not_exist.schema.json"), errorString);
    QVERIFY(!ok);
    QVERIFY(!errorString.isEmpty());
}

UT_REGISTER_TEST_LIGHTWEIGHT(JsonSchemaValidatorTest, TestLabel::Unit, TestLabel::Utilities)
