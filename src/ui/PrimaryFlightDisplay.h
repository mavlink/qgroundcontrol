#ifndef PRIMARYFLIGHTDISPLAY_H
#define PRIMARYFLIGHTDISPLAY_H

#include <QWidget>
#include <QPen>
#include "UASInterface.h"

class PrimaryFlightDisplay : public QWidget
{
    Q_OBJECT
public:
    PrimaryFlightDisplay(int width = 640, int height = 480, QWidget* parent = NULL);
    ~PrimaryFlightDisplay();

public slots:
    /** @brief Attitude from main autopilot / system state */
    void updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp);
    /** @brief Attitude from one specific component / redundant autopilot */
    void updateAttitude(UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 timestamp);

    void updatePrimarySpeed(UASInterface* uas, double speed, quint64 timstamp);
    void updateGPSSpeed(UASInterface* uas, double speed, quint64 timstamp);
    void updateClimbRate(UASInterface* uas, double altitude, quint64 timestamp);
    void updatePrimaryAltitude(UASInterface* uas, double altitude, quint64 timestamp);
    void updateGPSAltitude(UASInterface* uas, double altitude, quint64 timestamp);
    void updateNavigationControllerErrors(UASInterface* uas, double altitudeError, double speedError, double xtrackError);

    /** @brief Set the currently monitored UAS */
    virtual void setActiveUAS(UASInterface* uas);

protected:
    enum Layout {
        COMPASS_INTEGRATED,
        COMPASS_SEPARATED               // For a very high container. Feature panels are at bottom.
    };

    enum Style {
        NO_OVERLAYS,                    // Hzon not visible through tapes nor through feature panels. Frames with margins between.
        OVERLAY_HORIZONTAL,             // Hzon visible through tapes and (frameless) feature panels.
        OVERLAY_HSI                     // Hzon visible through everything except bottom feature panels.
    };

    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *e);

    // from HUD.h:

    /** @brief Preferred Size */
    QSize sizeHint() const;
    /** @brief Start updating widget */
    void showEvent(QShowEvent* event);
    /** @brief Stop updating widget */
    void hideEvent(QHideEvent* event);

    // dongfang: We have no context menu. Viewonly.
    // void contextMenuEvent (QContextMenuEvent* event);

    // dongfang: What is that?
    // dongfang: OK it's for UI interaction. Presently, there is none.
    void createActions();

signals:
    void visibilityChanged(bool visible);

private:
    enum AltimeterMode {
        PRIMARY_MAIN_GPS_SUB,   // Show the primary alt. on tape and GPS as extra info
        GPS_MAIN                // Show GPS on tape and no extra info
    };

    enum AltimeterFrame {
        ASL,                    // Show ASL altitudes (plane pilots' normal preference)
        RELATIVE_TO_HOME        // Show relative-to-home altitude (copter pilots)
    };

    enum SpeedMode {
        PRIMARY_MAIN_GROUND_SUB,// Show primary speed (often airspeed) on tape and groundspeed as extra
        GROUND_MAIN             // Show groundspeed on tape and no extra info
    };

    /*
     * There are at least these differences between airplane and copter PDF view:
     * - Airplane show absolute altutude in altimeter, copter shows relative to home
     */
    bool isAirplane();
    bool shouldDisplayNavigationData();

    void drawTextCenter(QPainter& painter, QString text, float fontSize, float x, float y);
    void drawTextLeftCenter(QPainter& painter, QString text, float fontSize, float x, float y);
    void drawTextRightCenter(QPainter& painter, QString text, float fontSize, float x, float y);
    void drawTextCenterBottom(QPainter& painter, QString text, float fontSize, float x, float y);
    void drawTextCenterTop(QPainter& painter, QString text, float fontSize, float x, float y);
    void drawAIGlobalFeatures(QPainter& painter, QRectF mainArea, QRectF paintArea);
    void drawAIAirframeFixedFeatures(QPainter& painter, QRectF area);
    void drawPitchScale(QPainter& painter, QRectF area, float intrusion, bool drawNumbersLeft, bool drawNumbersRight);
    void drawRollScale(QPainter& painter, QRectF area, bool drawTicks, bool drawNumbers);
    void drawAIAttitudeScales(QPainter& painter, QRectF area, float intrusion);
    void drawAICompassDisk(QPainter& painter, QRectF area, float halfspan);
    void drawSeparateCompassDisk(QPainter& painter, QRectF area);

    void drawAltimeter(QPainter& painter, QRectF area, float altitude, float secondaryAltitude, float vv);
    void drawVelocityMeter(QPainter& painter, QRectF area, float speed, float secondarySpeed);
    void fillInstrumentBackground(QPainter& painter, QRectF edge);
    void fillInstrumentOpagueBackground(QPainter& painter, QRectF edge);
    void drawInstrumentBackground(QPainter& painter, QRectF edge);

    /* This information is not currently included. These headers left in as a memo for restoration later.
    void drawLinkStatsPanel(QPainter& painter, QRectF area);
    void drawSysStatsPanel(QPainter& painter, QRectF area);
    void drawMissionStatsPanel(QPainter& painter, QRectF area);
    void drawSensorsStatsPanel(QPainter& painter, QRectF area);
    */

    void doPaint();

    UASInterface* uas;          ///< The uas currently monitored

    AltimeterMode altimeterMode;
    AltimeterFrame altimeterFrame;
    SpeedMode speedMode;

    bool didReceivePrimaryAltitude;
    bool didReceivePrimarySpeed;

    float roll;
    float pitch;
    float heading;

    float primaryAltitude;
    float GPSAltitude;

    // APM: GPS and baro mix above home (GPS) altitude. This value comes from the GLOBAL_POSITION_INT message.
    // Do !!!NOT!!! ever do altitude calculations at the ground station. There are enough pitfalls already.
    // If the MP "set home altitude" button is migrated to here, it must set the UAS home altitude, not a GS-local one.
    float aboveHomeAltitude;

    float primarySpeed;
    float groundspeed;
    float verticalVelocity;

    float navigationAltitudeError;
    float navigationSpeedError;
    float navigationCrosstrackError;
    float navigationTargetBearing;

    Layout layout;      // The display layout.
    Style style;        // The AI style (tapes translusent or opague)


    QColor redColor;
    QColor amberColor;
    QColor greenColor;

    qreal lineWidth;
    qreal fineLineWidth;

    qreal smallTestSize;
    qreal mediumTextSize;
    qreal largeTextSize;

    // Globally used stuff only.
    QPen instrumentEdgePen;
    QBrush instrumentBackground;
    QBrush instrumentOpagueBackground;

    QFont font;

    QTimer* refreshTimer;       ///< The main timer, controls the update rate

    static const int tickValues[];
    static const QString compassWindNames[];

    static const int updateInterval = 40;


signals:
    
public slots:
    
};

#endif // PRIMARYFLIGHTDISPLAY_H
