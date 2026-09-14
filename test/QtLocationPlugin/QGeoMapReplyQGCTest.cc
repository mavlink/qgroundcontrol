#include "QGeoMapReplyQGCTest.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtLocation/private/qgeotilespec_p.h>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include "QGeoMapReplyQGC.h"

class AbortFinishingNetworkReply final : public QNetworkReply
{
public:
    explicit AbortFinishingNetworkReply(QObject* parent = nullptr) : QNetworkReply(parent)
    {
        open(QIODevice::ReadOnly);
    }

    void abort() final
    {
        _abortCalled = true;
        emit finished();
    }

    bool abortCalled() const { return _abortCalled; }

protected:
    qint64 readData(char*, qint64) final { return -1; }

private:
    bool _abortCalled = false;
};

void QGeoMapReplyQGCTest::_testDestructorWithActiveReply()
{
    QPointer<AbortFinishingNetworkReply> guardedReply;

    {
        QGeoTiledMapReplyQGC mapReply(nullptr, QNetworkRequest(), QGeoTileSpec());
        auto* const reply = new AbortFinishingNetworkReply(this);
        guardedReply = reply;
        mapReply._networkReply = reply;
        (void) connect(reply, &QNetworkReply::finished, &mapReply, &QGeoTiledMapReplyQGC::_networkReplyFinished);
    }

    QVERIFY(guardedReply);
    QVERIFY(guardedReply->abortCalled());
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(guardedReply.isNull());
}

void QGeoMapReplyQGCTest::_testDestroyedNetworkObjects()
{
    auto* manager = new QNetworkAccessManager;
    QGeoTiledMapReplyQGC mapReply(manager, QNetworkRequest(), QGeoTileSpec());
    mapReply._networkReply = new AbortFinishingNetworkReply(manager);
    delete manager;

    mapReply._cacheError(QGCMapTask::TaskType::taskFetchTile, QStringView());
    QCOMPARE(mapReply.error(), QGeoTiledMapReply::CommunicationError);
    QCOMPARE(mapReply.errorString(), QStringLiteral("Network Manager Not Available"));
}

UT_REGISTER_TEST(QGeoMapReplyQGCTest, TestLabel::Unit)
