#include <QApplication>
#include <QDir>
#include <QShowEvent>

#include <QDebug>
#include "UASManager.h"

#ifdef Q_OS_MAC
#include <QWebFrame>
#include <QWebPage>
#include "QGCWebPage.h"
#endif

#include "ui_QGCGoogleEarthView.h"
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

void QGCGoogleEarthView::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    if (!event->spontaneous())
    {
        if (event->type() == QEvent::Hide)
        {
            // Disable widget
            updateTimer->stop();
        }
        else if (event->type() == QEvent::Show)
        {
            // Enable widget, initialize on first run
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
        }
        updateTimer->start();
    }
}

void QGCGoogleEarthView::updateState()
{
    if (isVisible())
    {
#ifdef Q_OS_MAC
        if (webViewMac->page()->currentFrame()->evaluateJavaScript("isInitialized();").toBool())
        {
#endif
#ifdef _MSC_VER
            //        if (webViewMacWin->dynamicCall("Navigate(const QString&)","isInitialized();").toBool())
            //        {
#endif
            static bool initialized = false;
            if (!initialized)
            {
#ifdef Q_OS_MAC
                webViewMac->page()->currentFrame()->evaluateJavaScript("setGCSHome(22.679833,8.549444, 470);");
#endif
#ifdef _MSC_VER
                //webViewMac->page()->currentFrame()->evaluateJavaScript("setGCSHome(22.679833,8.549444, 470);");
#endif
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
            #ifdef Q_OS_MAC
            webViewMac->page()->currentFrame()->evaluateJavaScript(QString("setAircraftPositionAttitude(%1, %2, %3, %4, %6, %7, %8);")
                                                                   .arg(uasId)
                                                                   .arg(lat)
                                                                   .arg(lon)
                                                                   .arg(alt+500)
                                                                   .arg(roll)
                                                                   .arg(pitch)
                                                                   .arg(yaw));
#endif
#ifdef _MSC_VER

#endif

            if (followCamera)
            {
#ifdef Q_OS_MAC
                webViewMac->page()->currentFrame()->evaluateJavaScript(QString("updateFollowAircraft()"));
#endif
#ifdef _MSC_VER
#endif
            }
#if (defined Q_OS_MAC) || (defined _MSC_VER)
        }
#endif
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
