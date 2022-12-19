/****************************************************************************
 *
 * (c) 2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include "ComponentInformationTranslation.h"

#include "UnitTest.h"

#include <QString>
#include <QJsonDocument>
#include <QByteArray>

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

