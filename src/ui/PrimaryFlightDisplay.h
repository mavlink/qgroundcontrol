#ifndef PRIMARYFLIGHTDISPLAY_H
#define PRIMARYFLIGHTDISPLAY_H

#include <QWidget>
#include <QPen>
#include "UASInterface.h"

#define SEPARATE_COMPASS_ASPECTRATIO (3.0f/4.0f)

#define LINEWIDTH 0.0036f

//#define TAPES_TEXT_SIZE 0.028
//#define AI_TEXT_SIZE 0.040
//#define AI_TEXT_MIN_PIXELS 12
//#define AI_TEXT_MAX_PIXELS 36
//#define PANELS_TEXT_SIZE 0.030
//#define COMPASS_SCALE_TEXT_SIZE 0.16

#define SMALL_TEXT_SIZE 0.03f
#define MEDIUM_TEXT_SIZE (SMALL_TEXT_SIZE*1.2f)
#define LARGE_TEXT_SIZE (MEDIUM_TEXT_SIZE*1.2f)

#define SHOW_ZERO_ON_SCALES true

// all in units of display height
#define ROLL_SCALE_RADIUS 0.42f
#define ROLL_SCALE_TICKMARKLENGTH 0.04f
#define ROLL_SCALE_MARKERWIDTH 0.06f
#define ROLL_SCALE_MARKERHEIGHT 0.04f
// scale max. degrees
#define ROLL_SCALE_RANGE 60

// fraction of height to translate for each degree of pitch.
#define PITCHTRANSLATION 65.0
// 10 degrees for each line
#define PITCH_SCALE_RESOLUTION 5
#define PITCH_SCALE_MAJORWIDTH 0.1
#define PITCH_SCALE_MINORWIDTH 0.066

// Beginning from PITCH_SCALE_WIDTHREDUCTION_FROM degrees of +/- pitch, the
// width of the lines is reduced, down to PITCH_SCALE_WIDTHREDUCTION times
// the normal width. This helps keep orientation in extreme attitudes.
#define PITCH_SCALE_WIDTHREDUCTION_FROM 30
#define PITCH_SCALE_WIDTHREDUCTION 0.3

#define PITCH_SCALE_HALFRANGE 15

// The number of degrees to either side of the heading to draw the compass disk.
// 180 is valid, this will draw a complete disk. If the disk is partly clipped
// away, less will do.

#define COMPASS_DISK_MAJORTICK 10
#define COMPASS_DISK_ARROWTICK 45
#define COMPASS_DISK_MAJORLINEWIDTH 0.006
#define COMPASS_DISK_MINORLINEWIDTH 0.004
#define COMPASS_DISK_RESOLUTION 10
#define COMPASS_SEPARATE_DISK_RESOLUTION 5
#define COMPASS_DISK_MARKERWIDTH 0.2
#define COMPASS_DISK_MARKERHEIGHT 0.133

#define TAPE_GAUGES_TICKWIDTH_MAJOR 0.25
#define TAPE_GAUGES_TICKWIDTH_MINOR 0.15

// The altitude difference between top and bottom of scale
#define ALTIMETER_LINEAR_SPAN 50
// every 5 meters there is a tick mark
#define ALTIMETER_LINEAR_RESOLUTION 5
// every 10 meters there is a number
#define ALTIMETER_LINEAR_MAJOR_RESOLUTION 10

// Projected: An experiment. Make tape appear projected from a cylinder, like a French "drum" style gauge.
// The altitude difference between top and bottom of scale
#define ALTIMETER_PROJECTED_SPAN 50
// every 5 meters there is a tick mark
#define ALTIMETER_PROJECTED_RESOLUTION 5
// every 10 meters there is a number
#define ALTIMETER_PROJECTED_MAJOR_RESOLUTION 10
// min. and max. vertical velocity
//#define ALTIMETER_PROJECTED

// min. and max. vertical velocity
#define ALTIMETER_VVI_SPAN 5
#define ALTIMETER_VVI_WIDTH 0.2

// Now the same thing for airspeed!
#define AIRSPEED_LINEAR_SPAN 15
#define AIRSPEED_LINEAR_RESOLUTION 1
#define AIRSPEED_LINEAR_MAJOR_RESOLUTION 5

#define UNKNOWN_BATTERY -1
#define UNKNOWN_ATTITUDE 0
#define UNKNOWN_ALTITUDE -1000
#define UNKNOWN_SPEED -1
#define UNKNOWN_COUNT -1
#define UNKNOWN_GPSFIXTYPE -1

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

    void updateSpeed(UASInterface* uas, SpeedMeasurementSource source, double speed, quint64 timstamp);
    void updateClimbRate(UASInterface* uas, AltitudeMeasurementSource source, double altitude, quint64 timestamp);
    void updateAltitude(UASInterface* uas, AltitudeMeasurementSource source, double altitude, quint64 timestamp);
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
