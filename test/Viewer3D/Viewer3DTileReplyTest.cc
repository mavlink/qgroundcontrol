/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "Viewer3DTileReplyTest.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkProxy>

#include "MapProvider.h"
#include "QGCMapUrlEngine.h"
#include "Viewer3DTileReply.h"

// Regression test for a destruction-order crash: Viewer3DTileQuery creates its
// QNetworkAccessManager child before its Viewer3DTileReply children. QObject
// deletes children in creation order, so the manager (and the QNetworkReply
// objects it owns) dies first, leaving each Viewer3DTileReply destructor with a
// dangling reply pointer unless it is guarded (QPointer).
void Viewer3DTileReplyTest::_testDestructionOrderNoCrash()
{
    const auto &providers = UrlFactory::getProviders();
    QVERIFY(!providers.isEmpty());
    const SharedMapProvider provider = providers.first();

    QObject *parentObj = new QObject();

    // Manager created first: it is destroyed before the tile reply below
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(parentObj);
    // Broken proxy keeps the request local so no real network traffic occurs
    networkManager->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, QStringLiteral("127.0.0.1"), 1));

    Viewer3DTileReply *tileReply = new Viewer3DTileReply(10, 1, 2, provider->getMapId(), provider->getMapName(), networkManager, parentObj);

    // Force the network path synchronously so _reply is live
    tileReply->_onCacheMiss();
    QVERIFY(tileReply->_reply);

    // Deletes the manager (and its QNetworkReply children) first, then the tile reply
    delete parentObj;
}

UT_REGISTER_TEST(Viewer3DTileReplyTest, TestLabel::Unit)
