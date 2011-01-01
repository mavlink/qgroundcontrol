#include <QApplication>
#include <QDir>
#include <QShowEvent>
#include <QSettings>

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include "UASManager.h"

#ifdef Q_OS_MAC
#include <QWebFrame>
#include <QWebPage>
#include "QGCWebPage.h"
#endif

#ifdef _MSC_VER
#include <QAxObject>
#include <QUuid>
#include <mshtml.h>
#endif

#include "QGC.h"
#include "ui_QGCGoogleEarthView.h"
#include "QGCGoogleEarthView.h"

#define QGCGOOGLEEARTHVIEWSETTINGS QString("GoogleEarthViewSettings_")

QGCGoogleEarthView::QGCGoogleEarthView(QWidget *parent) :
        QWidget(parent),
        updateTimer(new QTimer(this)),
        refreshRateMs(40),
        mav(NULL),
        followCamera(true),
        trailEnabled(true),
        webViewInitialized(false),
        jScriptInitialized(false),
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


    QFile file("doc.html");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        qDebug() << __FILE__ << __LINE__ << "Could not open log file";

    QTextStream out(&file);
    out << webViewWin->generateDocumentation();
    out.flush();
    file.flush();
    file.close();


#else
#endif

    // Load settings
    QSettings settings;
    followCamera = settings.value(QGCGOOGLEEARTHVIEWSETTINGS + "follow", followCamera).toBool();
    trailEnabled = settings.value(QGCGOOGLEEARTHVIEWSETTINGS + "trail", trailEnabled).toBool();

    ui->setupUi(this);
#if (defined Q_OS_MAC)
    ui->webViewLayout->addWidget(webViewMac);
    //connect(webViewMac, SIGNAL(loadFinished(bool)), this, SLOT(initializeGoogleEarth(bool)));
#endif

#ifdef _MSC_VER
    ui->webViewLayout->addWidget(webViewWin);
#endif

    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateState()));
}

QGCGoogleEarthView::~QGCGoogleEarthView()
{
    QSettings settings;
    settings.setValue(QGCGOOGLEEARTHVIEWSETTINGS + "follow", followCamera);
    settings.setValue(QGCGOOGLEEARTHVIEWSETTINGS + "trail", trailEnabled);
    settings.sync();
#if (defined Q_OS_MAC)
        delete webViewMac;
#endif
#ifdef _MSC_VER
        delete webViewWin;
#endif
    delete ui;
}

/**
 * @param range in centimeters
 */
void QGCGoogleEarthView::setViewRangeScaledInt(int range)
{
    setViewRange(range/100.0f);
}

/**
 * @param range in meters (SI-units)
 */
void QGCGoogleEarthView::setViewRange(float range)
{
    javaScript(QString("setViewRange(%1);").arg(range, 0, 'f', 5));
}

void QGCGoogleEarthView::addUAS(UASInterface* uas)
{
    // uasid, type, color (in #rrbbgg format)
    QString uasColor = uas->getColor().name().remove(0, 1);
    // Convert to bbggrr format
    QString rChannel = uasColor.mid(0, 2);
    uasColor.remove(0, 2);
    uasColor.prepend(rChannel);
    // Set alpha value to 0x66, append JavaScript quotes ('')
    uasColor.prepend("'66").append("'");
    javaScript(QString("createAircraft(%1, %2, %3);").arg(uas->getUASID()).arg(uas->getSystemType()).arg(uasColor));

    if (trailEnabled) javaScript(QString("showTrail(%1);").arg(uas->getUASID()));

    //javaScript(QString("createAircraft(%1, %2, %3);").arg(uas->getUASID()).arg(uas->getSystemType()).arg("0"));
    // Automatically receive further position updates
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
}

void QGCGoogleEarthView::setActiveUAS(UASInterface* uas)
{
    if (uas)
    {
        mav = uas;
        javaScript(QString("setCurrAircraft(%1);").arg(uas->getUASID()));
    }
}

void QGCGoogleEarthView::updateGlobalPosition(UASInterface* uas, double lon, double lat, double alt, quint64 usec)
{
    Q_UNUSED(usec);
    javaScript(QString("addTrailPosition(%1, %2, %3, %4);").arg(uas->getUASID()).arg(lat, 0, 'f', 15).arg(lon, 0, 'f', 15).arg(alt, 0, 'f', 15));

    //qDebug() << QString("addTrailPosition(%1, %2, %3, %4);").arg(uas->getUASID()).arg(lat, 0, 'f', 15).arg(lon, 0, 'f', 15).arg(alt, 0, 'f', 15);
}

void QGCGoogleEarthView::showTrail(bool state)
{
    // Check if the current trail has to be hidden
    if (trailEnabled && !state)
    {
        QList<UASInterface*> mavs = UASManager::instance()->getUASList();
        foreach (UASInterface* currMav, mavs)
        {
            javaScript(QString("hideTrail(%1);").arg(currMav->getUASID()));
        }
    }

    // Check if the current trail has to be shown
    if (!trailEnabled && state)
    {
        QList<UASInterface*> mavs = UASManager::instance()->getUASList();
        foreach (UASInterface* currMav, mavs)
        {
            javaScript(QString("showTrail(%1);").arg(currMav->getUASID()));
        }
    }
    trailEnabled = state;
    ui->trailCheckbox->setChecked(state);
}

void QGCGoogleEarthView::showWaypoints(bool state)
{
    waypointsEnabled = state;
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
    javaScript("goHome();");
}

void QGCGoogleEarthView::setHome(double lat, double lon, double alt)
{
    javaScript(QString("setGCSHome(%1,%2,%3);").arg(lat, 0, 'f', 15).arg(lon, 0, 'f', 15).arg(alt, 0, 'f', 15));
}

void QGCGoogleEarthView::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    updateTimer->stop();
}

void QGCGoogleEarthView::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    Q_UNUSED(event)
        // Enable widget, initialize on first run

        if (!webViewInitialized)
        {
#if (defined Q_OS_MAC)
            webViewMac->setPage(new QGCWebPage(webViewMac));
            webViewMac->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
            webViewMac->load(QUrl(QCoreApplication::applicationDirPath()+"/earth.html"));
#endif

#ifdef _MSC_VER
            //webViewWin->dynamicCall("GoHome()");
            webViewWin->dynamicCall("Navigate(const QString&)", QApplication::applicationDirPath() + "/earth.html");
#endif

            webViewInitialized = true;
            // Reloading the webpage, this resets Google Earth
            gEarthInitialized = false;

            QTimer::singleShot(3000, this, SLOT(initializeGoogleEarth()));
        }
        else
        {
            updateTimer->start(refreshRateMs);
        }
}

void QGCGoogleEarthView::printWinException(int no, QString str1, QString str2, QString str3)
{
    qDebug() << no << str1 << str2 << str3;
}

QVariant QGCGoogleEarthView::javaScript(QString javaScript)
{
#ifdef Q_OS_MAC
    return webViewMac->page()->currentFrame()->evaluateJavaScript(javaScript);
#endif
#ifdef _MSC_VER
    if(!jScriptInitialized)
    {
        qDebug() << "TOO EARLY JAVASCRIPT CALL, ABORTING";
        return QVariant(false);
    }
    else
    {
        QVariantList params;
        params.append(javaScript);
        params.append("JScript");
        return jScriptWin->dynamicCall("execScript(QString, QString)", params);
    }
#endif
}

void QGCGoogleEarthView::initializeGoogleEarth()
{
    if (!jScriptInitialized)
    {
#ifdef Q_OS_MAC
        jScriptInitialized = true;
#endif
#ifdef _MSC_VER
        QAxObject* doc = webViewWin->querySubObject("Document()");
        IDispatch* Disp;
        IDispatch* winDoc = NULL;

        //332C4425-26CB-11D0-B483-00C04FD90119 IHTMLDocument2
        //25336920-03F9-11CF-8FD0-00AA00686F13 HTMLDocument
        doc->queryInterface(QUuid("{332C4425-26CB-11D0-B483-00C04FD90119}"), (void**)(&winDoc));
        if (winDoc)
        {
            // Security:
            // CoInternetSetFeatureEnabled
            // (FEATURE_LOCALMACHINE_LOCKDOWN, SET_FEATURE_ON_PROCESS, TRUE);
            //
            IHTMLDocument2* document = NULL;
            winDoc->QueryInterface( IID_IHTMLDocument2, (void**)&document );
            IHTMLWindow2 *window = NULL;
            document->get_parentWindow( &window );

            jScriptWin = new QAxObject(window, webViewWin);
            connect(jScriptWin, SIGNAL(exception(int,QString,QString,QString)), this, SLOT(printWinException(int,QString,QString,QString)));
            jScriptInitialized = true;
        }
        else
        {
            qDebug() << "COULD NOT GET DOCUMENT OBJECT! Aborting";
        }
#endif
        QTimer::singleShot(3500, this, SLOT(initializeGoogleEarth()));
        return;
    }

    if (!gEarthInitialized)
    {
        if (0 == 1)//(!javaScript("isInitialized();").toBool())
        {
            QTimer::singleShot(500, this, SLOT(initializeGoogleEarth()));
            qDebug() << "NOT INITIALIZED, WAITING";
        }
        else
        {
            // Set home location
            setHome(47.3769, 8.549444, 500);

            // Move to home location
            goHome();

            // Add all MAVs
            QList<UASInterface*> mavs = UASManager::instance()->getUASList();
            foreach (UASInterface* currMav, mavs)
            {
                addUAS(currMav);
            }

            // Set current UAS
            setActiveUAS(UASManager::instance()->getActiveUAS());

            // Add any further MAV automatically
            connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)));
            connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)));

            // Connect UI signals/slots

            // Follow checkbox
            ui->followAirplaneCheckbox->setChecked(followCamera);
            connect(ui->followAirplaneCheckbox, SIGNAL(toggled(bool)), this, SLOT(follow(bool)));

            // Trail checkbox
            ui->trailCheckbox->setChecked(trailEnabled);
            connect(ui->trailCheckbox, SIGNAL(toggled(bool)), this, SLOT(showTrail(bool)));

            // Go home
            connect(ui->goHomeButton, SIGNAL(clicked()), this, SLOT(goHome()));

            // Cam distance slider
            connect(ui->camDistanceSlider, SIGNAL(valueChanged(int)), this, SLOT(setViewRangeScaledInt(int)));
            setViewRangeScaledInt(ui->camDistanceSlider->value());

            // Start update timer
            updateTimer->start(refreshRateMs);

            gEarthInitialized = true;
        }
    }
}

void QGCGoogleEarthView::updateState()
{
#if (QGC_EVENTLOOP_DEBUG)
    qDebug() << "EVENTLOOP:" << __FILE__ << __LINE__;
#endif
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
        foreach (UASInterface* currMav, mavs)
        {
            uasId = currMav->getUASID();
            lat = currMav->getLatitude();
            lon = currMav->getLongitude();
            alt = currMav->getAltitude();
            roll = currMav->getRoll();
            pitch = currMav->getPitch();
            yaw = currMav->getYaw();

            javaScript(QString("setAircraftPositionAttitude(%1, %2, %3, %4, %6, %7, %8);")
                       .arg(uasId)
                       .arg(lat, 0, 'f', 15)
                       .arg(lon, 0, 'f', 15)
                       .arg(alt, 0, 'f', 15)
                       .arg(roll, 0, 'f', 9)
                       .arg(pitch, 0, 'f', 9)
                       .arg(yaw, 0, 'f', 9));
        }

        if (followCamera)
        {
            javaScript(QString("updateFollowAircraft()"));
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
