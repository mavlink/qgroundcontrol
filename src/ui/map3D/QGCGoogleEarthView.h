#ifndef QGCGOOGLEEARTHVIEW_H
#define QGCGOOGLEEARTHVIEW_H

#include <QWidget>
#include <QTimer>
#include <UASInterface.h>

#if (defined Q_OS_MAC)
#include <QWebView>
#endif

#ifdef _MSC_VER
#include <ActiveQt/QAxWidget>
#include <ActiveQt/QAxObject>
#include "windows.h"

class QGCWebAxWidget : public QAxWidget
{
public:
    //Q_OBJECT
    QGCWebAxWidget(QWidget* parent = 0, Qt::WindowFlags f = 0)
        : QAxWidget(parent, f)/*,
		_document(NULL)*/
    {
        // Set web browser control
        setControl("{8856F961-340A-11D0-A96B-00C04FD705A2}");
        // WARNING: Makes it impossible to actually debug javascript. But useful in production mode
        setProperty("ScriptErrorsSuppressed", true);
        // see: http://www.codeproject.com/KB/cpp/ExtendedWebBrowser.aspx?fid=285594&df=90&mpp=25&noise=3&sort=Position&view=Quick&fr=151#GoalScriptError

        //this->dynamicCall("setProperty(const QString&,
        //QObject::connect(this, SIGNAL(DocumentComplete(IDispatch*, QVariant&)), this, SLOT(setDocument(IDispatch*, QVariant&)));


    }
    /*
    QAxObject* document()
    {
    	return _document;
    }*/

protected:
    /*
    void setDocument(IDispatch* dispatch, QVariant& variant)
    {
    	_document = this->querySubObject("Document()");
    }
    QAxObject* _document;*/
    virtual bool translateKeyEvent(int message, int keycode) const {
        if (message >= WM_KEYFIRST && message <= WM_KEYLAST)
            return true;
        else
            return QAxWidget::translateKeyEvent(message, keycode);
    }

};
#endif

namespace Ui
{
class QGCGoogleEarthView;
}

class Waypoint;

class QGCGoogleEarthView : public QWidget
{
    Q_OBJECT

public:
    explicit QGCGoogleEarthView(QWidget *parent = 0);
    ~QGCGoogleEarthView();

    enum VIEW_MODE {
        VIEW_MODE_SIDE = 0, ///< View from above, orientation is free
        VIEW_MODE_MAP, ///< View from above, orientation is north (map view)
        VIEW_MODE_CHASE_LOCKED, ///< Locked chase camera
        VIEW_MODE_CHASE_FREE, ///< Position is chasing object, but view can rotate around object
    };

public slots:
    /** @brief Update the internal state. Does not trigger a redraw */
    void updateState();
    /** @brief Add a new MAV/UAS to the visualization */
    void addUAS(UASInterface* uas);
    /** @brief Set the currently selected UAS */
    void setActiveUAS(UASInterface* uas);
    /** @brief Update the global position */
    void updateGlobalPosition(UASInterface* uas, double lat, double lon, double alt, quint64 usec);
    /** @brief Update a single waypoint */
    void updateWaypoint(int uas, Waypoint* wp);
    /** @brief Update the waypoint list */
    void updateWaypointList(int uas);
    /** @brief Show the vehicle trail */
    void showTrail(bool state);
    /** @brief Clear the current vehicle trails and start with new ones */
    void clearTrails();
    /** @brief Show the waypoints */
    void showWaypoints(bool state);
    /** @brief Follow the aircraft during flight */
    void follow(bool follow);
    /** @brief Go to the home location */
    void goHome();
    /** @brief Set the home location */
    void setHome(double lat, double lon, double alt);
    /** @brief Set the home location interactively in the UI */
    void setHome();
    /** @brief Move the view to a latitude / longitude position */
    void moveToPosition();
    /** @brief Allow waypoint editing */
    void enableEditMode(bool mode);
    /** @brief Enable daylight/night */
    void enableDaylight(bool enable);
    /** @brief Enable atmosphere */
    void enableAtmosphere(bool enable);
    /** @brief Set camera view range to aircraft in meters */
    void setViewRange(float range);
    /** @brief Set the distance mode - either to ground or to MAV */
    void setDistanceMode(int mode);
    /** @brief Set the view mode */
    void setViewMode(VIEW_MODE mode);
    /** @brief Toggle through the different view modes */
    void toggleViewMode();
    /** @brief Set camera view range to aircraft in centimeters */
    void setViewRangeScaledInt(int range);
    /** @brief Reset Google Earth View */
    void reloadHTML();

    /** @brief Initialize Google Earth */
    void initializeGoogleEarth();
    /** @brief Print a Windows exception */
    void printWinException(int no, QString str1, QString str2, QString str3);

public:
    /** @brief Execute java script inside the Google Earth window */
    QVariant javaScript(QString javascript);
    /** @brief Get a document element */
    QVariant documentElement(QString name);

signals:
    void visibilityChanged(bool visible);

protected:
    void changeEvent(QEvent *e);
    QTimer* updateTimer;
    int refreshRateMs;
    UASInterface* mav;
    bool followCamera;
    bool trailEnabled;
    bool waypointsEnabled;
    bool webViewInitialized;
    bool jScriptInitialized;
    bool gEarthInitialized;
    VIEW_MODE currentViewMode;
#ifdef _MSC_VER
    QGCWebAxWidget* webViewWin;
    QAxObject* jScriptWin;
    QAxObject* documentWin;
#endif
#if (defined Q_OS_MAC)
    QWebView* webViewMac;
#endif

    /** @brief Start widget updating */
    void showEvent(QShowEvent* event);
    /** @brief Stop widget updating */
    void hideEvent(QHideEvent* event);

private:
#ifdef _MSC_VER
    Ui::QGCGoogleEarthView* ui;
#else
    Ui::QGCGoogleEarthView* ui;
#endif
};

#endif // QGCGOOGLEEARTHVIEW_H
