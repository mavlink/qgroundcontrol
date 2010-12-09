#include <QWebFrame>
#include <QWebPage>

#include <QDebug>

#include "QGCGoogleEarthView.h"
#include "QGCWebPage.h"
#include "UASManager.h"
#include "ui_QGCGoogleEarthControls.h"
#if (defined Q_OS_WIN) && !(defined __MINGW32__)
#include "ui_QGCGoogleEarthViewWin.h"
#else
#include "ui_QGCGoogleEarthView.h"
#endif

QGCGoogleEarthView::QGCGoogleEarthView(QWidget *parent) :
        QWidget(parent),
        updateTimer(new QTimer(this)),
        mav(NULL),
        followCamera(true),
        trailEnabled(true),
#if (defined Q_OS_MAC)
        webViewMac(new QWebView(this)),
#endif
#if (defined Q_OS_WIN) & !(defined __MINGW32__)
        webViewWin(new QGCWebAxWidget(this)),
#else
        ui(new Ui::QGCGoogleEarthView)
#endif
{
#if (defined Q_OS_WIN) & !(defined __MINGW32__)
    // Create layout and attach webViewWin
#else
#endif

    ui->setupUi(this);

#if (defined Q_OS_MAC)
    ui->webViewLayout->addWidget(webViewMac);
    webViewMac->setPage(new QGCWebPage(webViewMac));
    webViewMac->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    webViewMac->load(QUrl("earth.html"));
#endif

#if (defined Q_OS_WIN) & !(defined __MINGW32__)
    webViewWin->load(QUrl("earth.html"));
#endif

#if ((defined Q_OS_MAC) | ((defined Q_OS_WIN) & !(defined __MINGW32__)))
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateState()));
    updateTimer->start(200);
#endif

    // Follow checkbox
    ui->followAirplaneCheckbox->setChecked(followCamera);
    connect(ui->followAirplaneCheckbox, SIGNAL(toggled(bool)), this, SLOT(follow(bool)));

    // Trail checkbox
    ui->trailCheckbox->setChecked(trailEnabled);
    connect(ui->trailCheckbox, SIGNAL(toggled(bool)), this, SLOT(showTrail(bool)));

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

void QGCGoogleEarthView::showTrail(bool state)
{

}

void QGCGoogleEarthView::showWaypoints(bool state)
{

}

void QGCGoogleEarthView::follow(bool follow)
{
    followCamera = follow;
}

void QGCGoogleEarthView::updateState()
{
#ifdef Q_OS_MAC
    if (webViewMac->page()->currentFrame()->evaluateJavaScript("isInitialized();").toBool())
    {
        static bool initialized = false;
        if (!initialized)
        {
            webViewMac->page()->currentFrame()->evaluateJavaScript("setGCSHome(22.679833,8.549444, 470);");
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
        webViewMac->page()->currentFrame()->evaluateJavaScript(QString("setAircraftPositionAttitude(%1, %2, %3, %4, %6, %7, %8);")
                                                                .arg(uasId)
                                                                .arg(lat)
                                                                .arg(lon)
                                                                .arg(alt+500)
                                                                .arg(roll)
                                                                .arg(pitch)
                                                                .arg(yaw));

        if (followCamera)
        {
             webViewMac->page()->currentFrame()->evaluateJavaScript(QString("updateFollowAircraft()"));
        }
    }
#endif
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
