#pragma once

#include "TestFixtures.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QByteArray>

/// Unit tests for ComponentInformationTranslation.
/// Tests JSON translation using Qt .ts translation files.
class ComponentInformationTranslationTest : public OfflineTest
{
    Q_OBJECT

public:
    ComponentInformationTranslationTest() = default;
    ~ComponentInformationTranslationTest() override = default;

private slots:
    // Construction tests
    void _testConstruction();

    // Basic translation tests
    void _testBasicTranslation();
    void _testOutputFileCreated();

    // Structure translation tests
    void _testListElementsTranslated();
    void _testObjectKeysTranslated();
    void _testRecursiveDefinitionsTranslated();
    void _testGlobalTranslationsApplied();
    void _testSpecialCharactersHandled();

    // Edge case tests
    void _testEmptyTsFileReturnsEmpty();
    void _testNonExistentJsonReturnsEmpty();
    void _testNonExistentTsReturnsEmpty();

private:
    void readJson(const QByteArray& bytes, QJsonDocument& jsonDoc);
    QJsonValue getNestedValue(const QJsonObject& obj, const QStringList& path);
};
