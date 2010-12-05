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
            lat = mav->getLat();
            lon = mav->getLon();
            alt = mav->getAlt();
            roll = mav->getRoll();
            pitch = mav->getPitch();
            yaw = mav->getYaw();
        }
//        ui->webView->page()->currentFrame()->evaluateJavaScript(QString("setAircraftPosition(%1, %2, %3, %4);")
//                                                                .arg(uasId)
//                                                                .arg(lat)
//                                                                .arg(lon)
//                                                                .arg(alt));
//        //ui->webView->page()->currentFrame()->evaluateJavaScript(QString("drawAndCenter(%1, %2, %3, %4, '%5', %6, %7, %8, %9, %10, %11);").arg(lat).arg(lon).arg(alt).arg("true").arg("ff0000ff").arg("1").arg("true").arg("true").arg(yaw).arg(pitch).arg(roll));

        if (followCamera)
        {
             ui->webView->page()->currentFrame()->evaluateJavaScript(QString("updateFollowAircraft()"));
            //ui->webView->page()->currentFrame()->evaluateJavaScript(QString("followAircraft(%1);").arg(mav->getUASID()));
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
