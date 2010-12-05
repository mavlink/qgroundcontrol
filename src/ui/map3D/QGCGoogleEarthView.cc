#include <QWebFrame>
#include <QWebPage>

#include <QDebug>

#include "QGCGoogleEarthView.h"
#include "QGCWebPage.h"
#include "UASManager.h"
#if (defined Q_OS_WIN) && !(defined __MINGW32__)
#else
#include "ui_QGCGoogleEarthView.h"
#endif

QGCGoogleEarthView::QGCGoogleEarthView(QWidget *parent) :
        QWidget(parent),
        updateTimer(new QTimer(this)),
        mav(NULL),
        followCamera(true),
#if (defined Q_OS_WIN) && !(defined __MINGW32__)
        webViewWin(new QGCWebAxWidget(this)),
#else
        ui(new Ui::QGCGoogleEarthView)
#endif
{
#if (defined Q_OS_WIN) && !(defined __MINGW32__)
    // Create layout and attach webViewWin
#else
    ui->setupUi(this);
    ui->webView->setPage(new QGCWebPage(ui->webView));
    ui->webView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);

    ui->webView->load(QUrl("earth.html"));
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateState()));
    updateTimer->start(200);
#endif

    // Get list of available 3D models

    // Load HTML file

    // Parse for model links

    // Populate model list
}

QGCGoogleEarthView::~QGCGoogleEarthView()
{
    delete ui;
}

void QGCGoogleEarthView::setActiveUAS(UASInterface* uas)
{
    mav = uas;
}

void QGCGoogleEarthView::updateState()
{
    if (ui->webView->page()->currentFrame()->evaluateJavaScript("isInitialized();").toBool())
    {
        static bool initialized = false;
        if (!initialized)
        {
            ui->webView->page()->currentFrame()->evaluateJavaScript("setGCSHome(22.679833,8.549444, 470);");
            initialized = true;
        }
        int uasId = 0;
        double lat = 22.679833;
        double lon = 8.549444;
        double alt = 470.0;

        float roll = 0.0f;
        float pitch = 0.0f;
        float yaw = 0.0f;

        if (mav)
        {
            uasId = mav->getUASID();
            lat = mav->getLatitude();
            lon = mav->getLongitude();
            alt = mav->getAltitude();
            roll = mav->getRoll();
            pitch = mav->getPitch();
            yaw = mav->getYaw();
        }
        ui->webView->page()->currentFrame()->evaluateJavaScript(QString("setAircraftPositionAttitude(%1, %2, %3, %4, %6, %7, %8);")
                                                                .arg(uasId)
                                                                .arg(lat)
                                                                .arg(lon)
                                                                .arg(alt+500)
                                                                .arg(roll)
                                                                .arg(pitch)
                                                                .arg(yaw));

        if (followCamera)
        {
             ui->webView->page()->currentFrame()->evaluateJavaScript(QString("updateFollowAircraft()"));
        }
    }
}

void QGCGoogleEarthView::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
