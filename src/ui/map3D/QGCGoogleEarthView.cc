#include <QApplication>
#include <QDir>
#include <QShowEvent>
#include <QSettings>
#include <QInputDialog>

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include "UASManager.h"

#ifdef Q_OS_MAC
#include <QWebFrame>
#include <QWebPage>
#include <QWebElement>
#include "QGCWebPage.h"
#endif

#ifdef _MSC_VER
#include <QAxObject>
#include <QUuid>
#include <mshtml.h>
#include <comdef.h>
#include <qaxtypes.h>
#endif

#include "QGC.h"
#include "ui_QGCGoogleEarthView.h"
#include "QGCGoogleEarthView.h"
#include "UASWaypointManager.h"

#define QGCGOOGLEEARTHVIEWSETTINGS QString("GoogleEarthViewSettings_")

QGCGoogleEarthView::QGCGoogleEarthView(QWidget *parent) :
    QWidget(parent),
    updateTimer(new QTimer(this)),
    refreshRateMs(100),
    mav(NULL),
    followCamera(true),
    trailEnabled(true),
    webViewInitialized(false),
    jScriptInitialized(false),
    gEarthInitialized(false),
    currentViewMode(QGCGoogleEarthView::VIEW_MODE_SIDE),
#if (defined Q_OS_MAC)
    webViewMac(new QWebView(this)),
#endif
#ifdef _MSC_VER
    webViewWin(new QGCWebAxWidget(this)),
    documentWin(NULL),
#endif
    ui(new Ui::QGCGoogleEarthView)
{
#ifdef _MSC_VER
    // Create layout and attach webViewWin

//    QFile file("doc.html");
//    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
//        qDebug() << __FILE__ << __LINE__ << "Could not open log file";

//    QTextStream out(&file);
//    out << webViewWin->generateDocumentation();
//    out.flush();
//    file.flush();
//    file.close();


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
    connect(ui->resetButton, SIGNAL(clicked()), this, SLOT(reloadHTML()));
    connect(ui->changeViewButton, SIGNAL(clicked()), this, SLOT(toggleViewMode()));
    connect(ui->clearTrailsButton, SIGNAL(clicked()), this, SLOT(clearTrails()));
    connect(ui->atmosphereCheckBox, SIGNAL(clicked(bool)), this, SLOT(enableAtmosphere(bool)));
    connect(ui->daylightCheckBox, SIGNAL(clicked(bool)), this, SLOT(enableDaylight(bool)));

    connect(UASManager::instance(), SIGNAL(homePositionChanged(double,double,double)), this, SLOT(setHome(double,double,double)));
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

void QGCGoogleEarthView::reloadHTML()
{
    hide();
    webViewInitialized = false;
    jScriptInitialized = false;
    gEarthInitialized = false;
    show();
}

void QGCGoogleEarthView::enableEditMode(bool mode)
{
    javaScript(QString("setDraggingAllowed(%1);").arg(mode));
}

void QGCGoogleEarthView::enableDaylight(bool enable)
{
    javaScript(QString("enableDaylight(%1);").arg(enable));
}

void QGCGoogleEarthView::enableAtmosphere(bool enable)
{
    javaScript(QString("enableAtmosphere(%1);").arg(enable));
}

/**
 * @param range in meters (SI-units)
 */
void QGCGoogleEarthView::setViewRange(float range)
{
    javaScript(QString("setViewRange(%1);").arg(range, 0, 'f', 5));
}

void QGCGoogleEarthView::setDistanceMode(int mode)
{
    javaScript(QString("setDistanceMode(%1);").arg(mode));
}

void QGCGoogleEarthView::toggleViewMode()
{
    switch (currentViewMode) {
    case VIEW_MODE_MAP:
        setViewMode(VIEW_MODE_SIDE);
        break;
    case VIEW_MODE_SIDE:
        setViewMode(VIEW_MODE_MAP);
        break;
    case VIEW_MODE_CHASE_LOCKED:
        setViewMode(VIEW_MODE_CHASE_FREE);
        break;
    case VIEW_MODE_CHASE_FREE:
        setViewMode(VIEW_MODE_CHASE_LOCKED);
        break;
    }
}

void QGCGoogleEarthView::setViewMode(QGCGoogleEarthView::VIEW_MODE mode)
{
    switch (mode) {
    case VIEW_MODE_MAP:
        ui->changeViewButton->setText("Free View");
        break;
    case VIEW_MODE_SIDE:
        ui->changeViewButton->setText("Map View");
        break;
    case VIEW_MODE_CHASE_LOCKED:
        ui->changeViewButton->setText("Free Chase");
        break;
    case VIEW_MODE_CHASE_FREE:
        ui->changeViewButton->setText("Fixed Chase");
        break;
    }
    currentViewMode = mode;
    javaScript(QString("setViewMode(%1);").arg(mode));
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

    // Automatically receive further position updates
    connect(uas, SIGNAL(globalPositionChanged(UASInterface*,double,double,double,quint64)), this, SLOT(updateGlobalPosition(UASInterface*,double,double,double,quint64)));
    // Receive waypoint updates
    // Connect the waypoint manager / data storage to the UI
    connect(uas->getWaypointManager(), SIGNAL(waypointEditableListChanged(int)), this, SLOT(updateWaypointList(int)));
    connect(uas->getWaypointManager(), SIGNAL(waypointEditableChanged(int, Waypoint*)), this, SLOT(updateWaypoint(int,Waypoint*)));
    //connect(this, SIGNAL(waypointCreated(Waypoint*)), uas->getWaypointManager(), SLOT(addWaypointEditable(Waypoint*)));
    // TODO Update waypoint list on UI changes here
}

void QGCGoogleEarthView::setActiveUAS(UASInterface* uas)
{
    if (uas)
    {
        mav = uas;
        javaScript(QString("setCurrAircraft(%1);").arg(uas->getUASID()));
        updateWaypointList(uas->getUASID());
    }
}

/**
 * This function is called if a a single waypoint is updated and
 * also if the whole list changes.
 */
void QGCGoogleEarthView::updateWaypoint(int uas, Waypoint* wp)
{
    // Only accept waypoints in global coordinate frame
    if ((wp->getFrame() == MAV_FRAME_GLOBAL || wp->getFrame() == MAV_FRAME_GLOBAL_RELATIVE_ALT) && wp->isNavigationType())
    {
        // We're good, this is a global waypoint

        // Get the index of this waypoint
        // note the call to getGlobalFrameAndNavTypeIndexOf()
        // as we're only handling global waypoints
        int wpindex = UASManager::instance()->getUASForId(uas)->getWaypointManager()->getGlobalFrameAndNavTypeIndexOf(wp);
        // If not found, return (this should never happen, but helps safety)
        if (wpindex == -1)
        {
            return;
        }
        else
        {
            javaScript(QString("updateWaypoint(%1,%2,%3,%4,%5,%6);").arg(uas).arg(wpindex).arg(wp->getLatitude(), 0, 'f', 22).arg(wp->getLongitude(), 0, 'f', 22).arg(wp->getAltitude(), 0, 'f', 22).arg(wp->getAction()));
            //qDebug() << QString("updateWaypoint(%1,%2,%3,%4,%5,%6);").arg(uas).arg(wpindex).arg(wp->getLatitude(), 0, 'f', 18).arg(wp->getLongitude(), 0, 'f', 18).arg(wp->getAltitude(), 0, 'f', 18).arg(wp->getAction());
        }
    }
}

/**
 * Update the whole list of waypoints. This is e.g. necessary if the list order changed.
 * The UAS manager will emit the appropriate signal whenever updating the list
 * is necessary.
 */
void QGCGoogleEarthView::updateWaypointList(int uas)
{
    // Get already existing waypoints
    UASInterface* uasInstance = UASManager::instance()->getUASForId(uas);
    if (uasInstance)
    {
        // Get all waypoints, including non-global waypoints
        QVector<Waypoint*> wpList = uasInstance->getWaypointManager()->getGlobalFrameAndNavTypeWaypointList();

        // Trim internal list to number of global waypoints in the waypoint manager list
        javaScript(QString("updateWaypointListLength(%1,%2);").arg(uas).arg(wpList.count()));

        qDebug() << QString("updateWaypointListLength(%1,%2);").arg(uas).arg(wpList.count());

        // Load all existing waypoints into map view
        foreach (Waypoint* wp, wpList)
        {
            updateWaypoint(uas, wp);
        }
    }
}

void QGCGoogleEarthView::updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec)
{
    Q_UNUSED(usec);
    javaScript(QString("addTrailPosition(%1, %2, %3, %4);").arg(uas->getUASID()).arg(lat, 0, 'f', 22).arg(lon, 0, 'f', 22).arg(alt, 0, 'f', 22));
}

void QGCGoogleEarthView::clearTrails()
{
    QList<UASInterface*> mavs = UASManager::instance()->getUASList();
    foreach (UASInterface* currMav, mavs)
    {
        javaScript(QString("clearTrail(%1);").arg(currMav->getUASID()));
    }
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
    if (follow != followCamera)
    {
        if (follow)
        {
            setViewMode(VIEW_MODE_CHASE_LOCKED);
        }
        else
        {
            setViewMode(VIEW_MODE_SIDE);
        }
    }
    followCamera = follow;
    javaScript(QString("setFollowEnabled(%1)").arg(follow));
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
    qDebug() << "SETTING GCS HOME IN GOOGLE MAPS" << lat << lon << alt;
    javaScript(QString("setGCSHome(%1,%2,%3);").arg(lat, 0, 'f', 15).arg(lon, 0, 'f', 15).arg(alt, 0, 'f', 15));
}

void QGCGoogleEarthView::setHome()
{
    javaScript(QString("enableSetHomeMode();"));
}

void QGCGoogleEarthView::moveToPosition()
{
    bool ok;
    javaScript("sampleCurrentPosition();");
    double latitude = documentElement("currentCameraLatitude").toDouble();
    double longitude = documentElement("currentCameraLongitude").toDouble();
    QString text = QInputDialog::getText(this, tr("Please enter coordinates"),
                                         tr("Coordinates (Lat,Lon):"), QLineEdit::Normal,
                                         QString("%1,%2").arg(latitude).arg(longitude), &ok);
    if (ok && !text.isEmpty()) {
        QStringList split = text.split(",");
        if (split.length() == 2) {
            bool convert;
            double latitude = split.first().toDouble(&convert);
            ok &= convert;
            double longitude = split.last().toDouble(&convert);
            ok &= convert;

            if (ok) {
                javaScript(QString("setLookAtLatLon(%1,%2);").arg(latitude, 0, 'f', 15).arg(longitude, 0, 'f', 15));
            }
        }
    }
}

void QGCGoogleEarthView::hideEvent(QHideEvent* event)
{
    updateTimer->stop();
    QWidget::hideEvent(event);
    emit visibilityChanged(false);
}

void QGCGoogleEarthView::showEvent(QShowEvent* event)
{
    // React only to internal (pre-display)
    // events
    QWidget::showEvent(event);
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
    emit visibilityChanged(true);
}

void QGCGoogleEarthView::printWinException(int no, QString str1, QString str2, QString str3)
{
    qDebug() << no << str1 << str2 << str3;
}

QVariant QGCGoogleEarthView::javaScript(QString javaScript)
{
#ifdef Q_OS_MAC
    if (!jScriptInitialized) {
        return QVariant(false);
    } else {
        return webViewMac->page()->currentFrame()->evaluateJavaScript(javaScript);
    }
#endif
#ifdef _MSC_VER
    if(!jScriptInitialized) {
        qDebug() << "TOO EARLY JAVASCRIPT CALL, ABORTING";
        return QVariant(false);
    } else {
        QVariantList params;
        params.append(javaScript);
        params.append("JScript");
        QVariant result = jScriptWin->dynamicCall("execScript(QString, QString)", params);
        return result;
    }
#endif
}

QVariant QGCGoogleEarthView::documentElement(QString name)
{
#ifdef Q_OS_MAC
    QString javaScript("getGlobal(%1)");
    QVariant result = webViewMac->page()->currentFrame()->evaluateJavaScript(javaScript.arg(name));
    return result;
#endif
#ifdef _MSC_VER
    if(!jScriptInitialized) {
        qDebug() << "TOO EARLY JAVASCRIPT CALL, ABORTING";
    } else {
        if (documentWin) {
            QString resultString;

            // Get HTMLElement object
            QVariantList params;
            IHTMLDocument3* doc;
            documentWin->queryInterface( IID_IHTMLDocument3, (void**)&doc);
            params.append(name);
            IHTMLElement* element = NULL;
            // Append alias
            name.prepend("JScript_");
            HRESULT res = doc->getElementById(QStringToBSTR(name), &element);
            //BSTR elemString;
            if (SUCCEEDED(res) && element) {
                //element->get_innerHTML(&elemString);
                VARIANT var;
                var.vt = VT_BSTR;
                HRESULT res = element->getAttribute(L"value", 0, &var);
                if (SUCCEEDED(res) && (var.vt != VT_NULL)) {
                    QByteArray typeName;
                    QVariant qtValue = VARIANTToQVariant(var,typeName);
                    return qtValue;
                } else {
                    qDebug() << __FILE__ << __LINE__ << "JAVASCRIPT ATTRIBUTE" << name << "NOT FOUND";
                }
            } else {
                qDebug() << __FILE__ << __LINE__ << "DID NOT GET HTML ELEMENT" << name;
            }
        }
    }
    return QVariant(0);
#endif
}

void QGCGoogleEarthView::initializeGoogleEarth()
{
    if (!jScriptInitialized) {
#ifdef Q_OS_MAC
        jScriptInitialized = true;
#endif
#ifdef _MSC_VER
        QAxObject* doc = webViewWin->querySubObject("Document()");
        //IDispatch* Disp;
        IDispatch* winDoc = NULL;
        IHTMLDocument2* document = NULL;

        //332C4425-26CB-11D0-B483-00C04FD90119 IHTMLDocument2
        //25336920-03F9-11CF-8FD0-00AA00686F13 HTMLDocument
        doc->queryInterface(QUuid("{332C4425-26CB-11D0-B483-00C04FD90119}"), (void**)(&winDoc));
        if (winDoc) {
            document = NULL;
            winDoc->QueryInterface( IID_IHTMLDocument2, (void**)&document );
            IHTMLWindow2 *window = NULL;
            document->get_parentWindow( &window );
            documentWin = new QAxObject(document, webViewWin);
            jScriptWin = new QAxObject(window, webViewWin);
            connect(jScriptWin, SIGNAL(exception(int,QString,QString,QString)), this, SLOT(printWinException(int,QString,QString,QString)));
            jScriptInitialized = true;
        } else {
            qDebug() << "COULD NOT GET DOCUMENT OBJECT! Aborting";
        }
#endif
        QTimer::singleShot(1500, this, SLOT(initializeGoogleEarth()));
        return;
    }

    if (!gEarthInitialized) {
        if (!documentElement("initialized").toBool()) {
            QTimer::singleShot(300, this, SLOT(initializeGoogleEarth()));
            qDebug() << "NOT INITIALIZED, WAITING";
        } else {
            gEarthInitialized = true;

            // Set home location
            setHome(UASManager::instance()->getHomeLatitude(), UASManager::instance()->getHomeLongitude(), UASManager::instance()->getHomeAltitude());

            // Add all MAVs
            QList<UASInterface*> mavs = UASManager::instance()->getUASList();
            foreach (UASInterface* currMav, mavs) {
                addUAS(currMav);
            }

            // Set current UAS
            setActiveUAS(UASManager::instance()->getActiveUAS());

            // Add any further MAV automatically
            connect(UASManager::instance(), SIGNAL(UASCreated(UASInterface*)), this, SLOT(addUAS(UASInterface*)), Qt::UniqueConnection);
            connect(UASManager::instance(), SIGNAL(activeUASSet(UASInterface*)), this, SLOT(setActiveUAS(UASInterface*)), Qt::UniqueConnection);

            // Connect UI signals/slots

            // Follow checkbox
            ui->followAirplaneCheckbox->setChecked(followCamera);
            connect(ui->followAirplaneCheckbox, SIGNAL(toggled(bool)), this, SLOT(follow(bool)), Qt::UniqueConnection);

            // Trail checkbox
            ui->trailCheckbox->setChecked(trailEnabled);
            connect(ui->trailCheckbox, SIGNAL(toggled(bool)), this, SLOT(showTrail(bool)), Qt::UniqueConnection);

            // Go home
            connect(ui->goHomeButton, SIGNAL(clicked()), this, SLOT(goHome()));
            // Set home
            connect(ui->setHomeButton, SIGNAL(clicked()), this, SLOT(setHome()));

            // Visibility of set home button
            connect(ui->editButton, SIGNAL(clicked(bool)), ui->setHomeButton, SLOT(setVisible(bool)));
            ui->setHomeButton->setVisible(ui->editButton->isChecked());

            // To Lat/Lon button
            connect(ui->toLatLonButton, SIGNAL(clicked()), this, SLOT(moveToPosition()));

            // Cam distance slider
            connect(ui->camDistanceSlider, SIGNAL(valueChanged(int)), this, SLOT(setViewRangeScaledInt(int)), Qt::UniqueConnection);
            setViewRangeScaledInt(ui->camDistanceSlider->value());

            // Distance combo box
            connect(ui->camDistanceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setDistanceMode(int)), Qt::UniqueConnection);
            // Edit mode button
            connect(ui->editButton, SIGNAL(clicked(bool)), this, SLOT(enableEditMode(bool)), Qt::UniqueConnection);

            // Update waypoint list
            if (mav) updateWaypointList(mav->getUASID());

            // Start update timer
            updateTimer->start(refreshRateMs);

            // Set current view mode
            setViewMode(currentViewMode);
            setDistanceMode(ui->camDistanceComboBox->currentIndex());
            enableEditMode(ui->editButton->isChecked());
            enableAtmosphere(ui->atmosphereCheckBox->isChecked());
            enableDaylight(ui->daylightCheckBox->isChecked());
            follow(this->followCamera);

            // Move to home location
            goHome();
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
            // Only update if known
            if (!currMav->globalPositionKnown()) continue;
            uasId = currMav->getUASID();
            lat = currMav->getLatitude();
            lon = currMav->getLongitude();
            alt = currMav->getAltitude();
            roll = currMav->getRoll();
            pitch = currMav->getPitch();
            yaw = currMav->getYaw();

            //qDebug() << "SETTING POSITION FOR" << uasId << lat << lon << alt << roll << pitch << yaw;

            javaScript(QString("setAircraftPositionAttitude(%1, %2, %3, %4, %6, %7, %8);")
                       .arg(uasId)
                       .arg(lat, 0, 'f', 22)
                       .arg(lon, 0, 'f', 22)
                       .arg(alt, 0, 'f', 22)
                       .arg(roll, 0, 'f', 9)
                       .arg(pitch, 0, 'f', 9)
                       .arg(yaw, 0, 'f', 9));
        }


        // Read out new waypoint positions and waypoint create events
        // this is polling (bad) but forced because of the crappy
        // Microsoft API available in Qt - improvements wanted

        // First check if a new WP should be created
        bool newWaypointPending = documentElement("newWaypointPending").toBool();
        if (newWaypointPending)
        {
            bool coordsOk = true;
            bool ok;
            double latitude = documentElement("newWaypointLatitude").toDouble(&ok);
            coordsOk &= ok;
            double longitude = documentElement("newWaypointLongitude").toDouble(&ok);
            coordsOk &= ok;
            double altitude = documentElement("newWaypointAltitude").toDouble(&ok);
            coordsOk &= ok;
            if (coordsOk)
            {
                // Add new waypoint
                if (mav)
                {
                    Waypoint* wp = mav->getWaypointManager()->createWaypoint();
                    wp->setFrame(MAV_FRAME_GLOBAL);
                    wp->setAction(MAV_CMD_NAV_WAYPOINT);
                    wp->setLatitude(latitude);
                    wp->setLongitude(longitude);
                    wp->setAltitude(altitude);
                    wp->setAcceptanceRadius(10.0); // 10 m
                }
            }
            javaScript("setNewWaypointPending(false);");
        }

        // Check if a waypoint should be moved
        bool dragWaypointPending = documentElement("dragWaypointPending").toBool();

        if (dragWaypointPending)
        {
            bool coordsOk = true;
            bool ok;
            double latitude = documentElement("dragWaypointLatitude").toDouble(&ok);
            coordsOk &= ok;
            double longitude = documentElement("dragWaypointLongitude").toDouble(&ok);
            coordsOk &= ok;
            double altitude = documentElement("dragWaypointAltitude").toDouble(&ok);
            coordsOk &= ok;

            // UPDATE WAYPOINTS, HOME LOCATION AND OTHER LOCATIONS
            if (coordsOk)
            {
                QString idText = documentElement("dragWaypointIndex").toString();
                if (idText == "HOME")
                {
                    qDebug() << "HOME UPDATED!";
                    UASManager::instance()->setHomePosition(latitude, longitude, altitude);
                    ui->setHomeButton->setChecked(false);
                }
                else
                {
                    // Update waypoint or symbol
                    if (mav)
                    {
                        QVector<Waypoint*> wps = mav->getWaypointManager()->getGlobalFrameAndNavTypeWaypointList();

                        bool ok;
                        int index = idText.toInt(&ok);

                        if (ok && index >= 0 && index < wps.count())
                        {
                            Waypoint* wp = wps.at(index);
                            wp->setLatitude(latitude);
                            wp->setLongitude(longitude);
                            wp->setAltitude(altitude);
                            mav->getWaypointManager()->notifyOfChangeEditable(wp);
                        }
                    }
                }
            }
            else
            {
                // If coords were not ok, move the view in google earth back
                // to last acceptable location
                updateWaypointList(mav->getUASID());
            }
            javaScript("setDragWaypointPending(false);");
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
