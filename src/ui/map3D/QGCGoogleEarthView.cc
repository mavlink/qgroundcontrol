#include <QApplication>
#include <QDir>

#include <QDebug>
#include "UASManager.h"
#ifdef _MSC_VER
#include "ui_QGCGoogleEarthView.h"
#else
#include <QWebFrame>
#include <QWebPage>
#include "QGCWebPage.h"
#include "ui_QGCGoogleEarthView.h"
#endif
#include "QGCGoogleEarthView.h"

QGCGoogleEarthView::QGCGoogleEarthView(QWidget *parent) :
        QWidget(parent),
        updateTimer(new QTimer(this)),
        refreshRateMs(200),
        mav(NULL),
        followCamera(true),
        trailEnabled(true),
        webViewInitialized(false),
#if (defined Q_OS_MAC)
        webViewMac(new QWebView(this)),
#endif
#ifdef _MSC_VER
        webViewWin(new QGCWebAxWidget(this)),
#endif
#if (defined _MSC_VER)
		ui(new Ui::QGCGoogleEarthView)
#else
        ui(new Ui::QGCGoogleEarthView)
#endif
{
#ifdef _MSC_VER
    // Create layout and attach webViewWin
#else
#endif

    ui->setupUi(this);
#if (defined Q_OS_MAC)
    ui->webViewLayout->addWidget(webViewMac);
#endif

#ifdef _MSC_VER
	ui->webViewLayout->addWidget(webViewWin);
#endif

#if ((defined Q_OS_MAC) | (defined _MSC_VER))
    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateState()));
    updateTimer->start(refreshRateMs);
#endif

    // Follow checkbox
    ui->followAirplaneCheckbox->setChecked(followCamera);
    connect(ui->followAirplaneCheckbox, SIGNAL(toggled(bool)), this, SLOT(follow(bool)));

    // Trail checkbox
    ui->trailCheckbox->setChecked(trailEnabled);
    connect(ui->trailCheckbox, SIGNAL(toggled(bool)), this, SLOT(showTrail(bool)));

    // Get list of available 3D models

    // Load HTML file
	#ifdef _MSC_VER
	webViewWin->dynamicCall("GoHome()");
	webViewWin->dynamicCall("Navigate(const QString&)", QApplication::applicationDirPath() + "/earth.html");
#endif

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

void QGCGoogleEarthView::hide()
{
    updateTimer->stop();
    QWidget::hide();
}

void QGCGoogleEarthView::show()
{
    if (!webViewInitialized)
    {
#if (defined Q_OS_MAC)
    webViewMac->setPage(new QGCWebPage(webViewMac));
    webViewMac->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    webViewMac->load(QUrl("earth.html"));
#endif

#ifdef _MSC_VER
	webViewWin->dynamicCall("GoHome()");
	webViewWin->dynamicCall("Navigate(const QString&)", "http://pixhawk.ethz.ch");
#endif
    webViewInitialized = true;
    }
    updateTimer->start();
    QWidget::show();
}

void QGCGoogleEarthView::updateState()
{
#ifdef Q_OS_MAC
    if (isVisible())
    {
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
