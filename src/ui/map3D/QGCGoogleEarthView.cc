#include <QApplication>
#include <QDir>
#include <QShowEvent>
#include <QSettings>

#include <QDebug>
#include "UASManager.h"

#ifdef Q_OS_MAC
#include <QWebFrame>
#include <QWebPage>
#include "QGCWebPage.h"
#endif

#include "ui_QGCGoogleEarthView.h"
#include "QGCGoogleEarthView.h"

#define QGCGOOGLEEARTHVIEWSETTINGS QString("GoogleEarthViewSettings_")

QGCGoogleEarthView::QGCGoogleEarthView(QWidget *parent) :
        QWidget(parent),
        updateTimer(new QTimer(this)),
        refreshRateMs(80),
        mav(NULL),
        followCamera(true),
        trailEnabled(true),
        webViewInitialized(false),
        gEarthInitialized(false),
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

    // Load settings
    QSettings settings;
    followCamera = settings.value(QGCGOOGLEEARTHVIEWSETTINGS + "follow", followCamera).toBool();
    trailEnabled = settings.value(QGCGOOGLEEARTHVIEWSETTINGS + "trail", trailEnabled).toBool();

    ui->setupUi(this);
#if (defined Q_OS_MAC)
    ui->webViewLayout->addWidget(webViewMac);
    connect(webViewMac, SIGNAL(loadFinished(bool)), this, SLOT(initializeGoogleEarth(bool)));
#endif

#ifdef _MSC_VER
    ui->webViewLayout->addWidget(webViewWin);
#endif

    connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateState()));

    // Follow checkbox
    ui->followAirplaneCheckbox->setChecked(followCamera);
    connect(ui->followAirplaneCheckbox, SIGNAL(toggled(bool)), this, SLOT(follow(bool)));

    // Trail checkbox
    ui->trailCheckbox->setChecked(trailEnabled);
    connect(ui->trailCheckbox, SIGNAL(toggled(bool)), this, SLOT(showTrail(bool)));

    // Go home
    connect(ui->goHomeButton, SIGNAL(clicked()), this, SLOT(goHome()));
}

QGCGoogleEarthView::~QGCGoogleEarthView()
{
    QSettings settings;
    settings.setValue(QGCGOOGLEEARTHVIEWSETTINGS + "follow", followCamera);
    settings.setValue(QGCGOOGLEEARTHVIEWSETTINGS + "trail", trailEnabled);
    settings.sync();
    delete ui;
}

void QGCGoogleEarthView::addUAS(UASInterface* uas)
{
#ifdef Q_OS_MAC
        webViewMac->page()->currentFrame()->evaluateJavaScript(QString("createAircraft(%1, %2, %3);").arg(uas->getUASID()).arg(uas->getSystemType()).arg(uas->getColor().name()));
#endif
#ifdef _MSC_VER
        //if (webViewMac->page()->currentFrame()->evaluateJavaScript("isInitialized();").toBool())
#endif

        // Automatically receive further position updates
        connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
}

void QGCGoogleEarthView::setActiveUAS(UASInterface* uas)
{
    if (uas)
    {
        mav = uas;
#ifdef Q_OS_MAC
        if (webViewMac->page()->currentFrame()->evaluateJavaScript("isInitialized();").toBool())
        {
            webViewMac->page()->currentFrame()->evaluateJavaScript(QString("setCurrAircraft(%1);").arg(uas->getUASID()));
        }
#endif
#ifdef _MSC_VER
        //if (webViewMac->page()->currentFrame()->evaluateJavaScript("isInitialized();").toBool())
#endif
    }
}

void QGCGoogleEarthView::updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec)
{
    Q_UNUSED(usec);
#ifdef Q_OS_MAC
        webViewMac->page()->currentFrame()->evaluateJavaScript(QString("addTrailPosition(%1, %2, %3, %4);").arg(uas->getUASID()).arg(lat, 0, 'f', 15).arg(lon, 0, 'f', 15).arg(alt, 0, 'f', 15));
#endif
#ifdef _MSC_VER
        //if (webViewMac->page()->currentFrame()->evaluateJavaScript("isInitialized();").toBool())
#endif
}

void QGCGoogleEarthView::showTrail(bool state)
{
    ui->trailCheckbox->setChecked(state);
}

void QGCGoogleEarthView::showWaypoints(bool state)
{

}

void QGCGoogleEarthView::follow(bool follow)
{
    ui->followAirplaneCheckbox->setChecked(follow);
    followCamera = follow;
}

void QGCGoogleEarthView::goHome()
{
    // Disable follow and update
    follow(false);
    updateState();
    // Go to home location
#ifdef Q_OS_MAC
    webViewMac->page()->currentFrame()->evaluateJavaScript("goHome();");
#endif
#ifdef _MSC_VER
    webViewWin.dynamicCall("InvokeScript(\"goHome\");");
#endif
}

void QGCGoogleEarthView::setHome(double lat, double lon, double alt)
{
#ifdef Q_OS_MAC
    webViewMac->page()->currentFrame()->evaluateJavaScript(QString("setGCSHome(%1,%2,%3);").arg(lat, 0, 'f', 15).arg(lon, 0, 'f', 15).arg(alt, 0, 'f', 15));
#endif
#ifdef _MSC_VER
    webViewWin.dynamicCall(QString("InvokeScript(\"setGCSHome\", %1, %2, %3);").arg(lat, 0, 'f', 15).arg(lon, 0, 'f', 15).arg(alt, 0, 'f', 15));
#endif
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
            qDebug() << "STOPPED GOOGLE EARTH UPDATES";
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
                //webViewWin->dynamicCall("GoHome()");
                webViewWin->dynamicCall("Navigate(const QString&)", QApplication::applicationDirPath() + "/earth.html");
#endif

                webViewInitialized = true;
                // Reloading the webpage, this resets Google Earth
                gEarthInitialized = false;

                QTimer::singleShot(1000, this, SLOT(initializeGoogleEarth()));
                updateTimer->start(refreshRateMs);
            }
        }
    }
}

void QGCGoogleEarthView::initializeGoogleEarth()
{
    if (!gEarthInitialized)
    {
#ifdef Q_OS_MAC
        if (!webViewMac->page()->currentFrame()->evaluateJavaScript("isInitialized();").toBool())
#endif
#ifdef _MSC_VER
            //if (!webViewMac->page()->currentFrame()->evaluateJavaScript("isInitialized();").toBool())
#endif
        {
            QTimer::singleShot(200, this, SLOT(initializeGoogleEarth()));
        }
        else
        {
            // Set home location
            setHome(47.3769, 8.549444, 500);

            // Move to home location
            goHome();

            // Set current UAS
            setActiveUAS(mav);

            // Add all MAVs
            QList<UASInterface*> mavs = UASManager::instance()->getUASList();
            foreach (UASInterface* mav, mavs)
            {
                addUAS(mav);
            }

            // Add any further MAV automatically
            connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));

            gEarthInitialized = true;
        }
    }
}

void QGCGoogleEarthView::updateState()
{
    if (gEarthInitialized)
    {
        int uasId = 0;
        double lat = 47.3769;
        double lon = 8.549444;
        double alt = 470.0;

        float roll = 0.0f;
        float pitch = 0.0f;
        float yaw = 0.0f;

        // Update all MAVs
        QList<UASInterface*> mavs = UASManager::instance()->getUASList();
        foreach (UASInterface* mav, mavs)
        {
            uasId = mav->getUASID();
            lat = mav->getLatitude();
            lon = mav->getLongitude();
            alt = mav->getAltitude();
            roll = mav->getRoll();
            pitch = mav->getPitch();
            yaw = mav->getYaw();

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
        }

        if (followCamera)
        {
#ifdef Q_OS_MAC
            webViewMac->page()->currentFrame()->evaluateJavaScript(QString("updateFollowAircraft()"));
#endif
#ifdef _MSC_VER
#endif
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
