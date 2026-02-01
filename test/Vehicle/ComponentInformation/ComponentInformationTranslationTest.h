#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>

#include "UnitTest.h"

class ComponentInformationTranslationTest : public UnitTest
{
    Q_OBJECT

public:
    ComponentInformationTranslationTest() = default;
    virtual ~ComponentInformationTranslationTest() = default;

private slots:
    void _basic_test();

private:
    void readJson(const QByteArray& bytes, QJsonDocument& jsonDoc);
};
