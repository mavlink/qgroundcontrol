#include "QGCFileDownloadTest.h"
#include "QGCCachedFileDownload.h"
#include "QGCFileDownload.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

void QGCFileDownloadTest::_testFileDownload()
{
    // Local File
    QGCFileDownload *const downloader = new QGCFileDownload(this);
    (void) connect(downloader, &QGCFileDownload::downloadComplete, this,
        [this](const QString &remoteFile, const QString &localFile, const QString &errorMsg) {
            sender()->deleteLater();
            QVERIFY(errorMsg.isEmpty());
        });
    QSignalSpy spyQGCFileDownloadDownloadComplete(downloader, &QGCFileDownload::downloadComplete);
    QVERIFY(downloader->download(":/arducopter.apj"));
    QVERIFY(spyQGCFileDownloadDownloadComplete.wait(1000));
}

void QGCFileDownloadTest::_testCachedFileDownload()
{
    QGCCachedFileDownload file("");
}
