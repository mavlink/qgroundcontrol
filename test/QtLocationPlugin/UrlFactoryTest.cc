/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UrlFactoryTest.h"

#include "QGCMapUrlEngine.h"
#include "MapProvider.h"

#include <QtTest/QTest>

void UrlFactoryTest::_testUnknownMapIdFallback()
{
    // Choose a large id that should not collide with any known providers
    constexpr int unknownId = 987654321;

    QVERIFY(!UrlFactory::getMapProviderFromQtMapId(unknownId));

    const QString fallbackType = UrlFactory::getProviderTypeFromQtMapId(unknownId);
    QCOMPARE(fallbackType, QString::number(unknownId));
    QCOMPARE(UrlFactory::getQtMapIdFromProviderType(fallbackType), unknownId);
}

void UrlFactoryTest::_testNumericProviderLookup()
{
    const QList<SharedMapProvider> providers = UrlFactory::getProviders();
    QVERIFY(!providers.isEmpty());

    const SharedMapProvider provider = providers.first();
    QVERIFY(provider);

    const int providerId = provider->getMapId();
    QVERIFY(providerId != UrlFactory::defaultSetMapId());

    const QString numericType = QString::number(providerId);
    const SharedMapProvider resolvedProvider = UrlFactory::getMapProviderFromProviderType(numericType);
    QVERIFY(resolvedProvider);
    QCOMPARE(resolvedProvider->getMapId(), providerId);
}
