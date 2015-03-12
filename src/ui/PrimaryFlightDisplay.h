#ifndef PRIMARYFLIGHTDISPLAY_H
#define PRIMARYFLIGHTDISPLAY_H

#include <QWidget>
#include <QPen>
#include "UASInterface.h"

class PrimaryFlightDisplay : public QWidget
{
    Q_OBJECT
public:
    PrimaryFlightDisplay(QWidget* parent = NULL);
    ~PrimaryFlightDisplay();

public slots:
    /** @brief Attitude from main autopilot / system state */
    void updateAttitude(UASInterface* uas, double roll, double pitch, double yaw, quint64 timestamp);
    /** @brief Attitude from one specific component / redundant autopilot */
    void updateAttitude(UASInterface* uas, int component, double roll, double pitch, double yaw, quint64 timestamp);

    void updateSpeed(UASInterface* uas, double _groundSpeed, double _airSpeed, quint64 timestamp);
    void updateAltitude(UASInterface* uas, double _altitudeAMSL, double _altitudeWGS84, double _altitudeRelative, double _climbRate, quint64 timestamp);
    void updateNavigationControllerErrors(UASInterface* uas, double altitudeError, double speedError, double xtrackError);
    void UpdateNavigationControllerData(UASInterface *uas, float navRoll, float navPitch, float navBearing, float targetBearing, float targetDistance);

    /** @brief Set the currently monitored UAS */
    void forgetUAS(UASInterface* uas);
    void setActiveUAS(UASInterface* uas);

    void checkUpdate();

protected:

    bool _valuesChanged;
    quint64 _valuesLastPainted;

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

signals:
    void visibilityChanged(bool visible);

private:
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

    void drawAltimeter(QPainter& painter, QRectF area);
    void drawVelocityMeter(QPainter& painter, QRectF area);
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

    bool didReceiveSpeed;

    float roll;
    float pitch;
    float heading;

    float altitudeAMSL;
    float altitudeWGS84;
    float altitudeRelative;

    // APM: GPS and baro mix above home (GPS) altitude. This value comes from the GLOBAL_POSITION_INT message.
    // Do !!!NOT!!! ever do altitude calculations at the ground station. There are enough pitfalls already.
    // If the MP "set home altitude" button is migrated to here, it must set the UAS home altitude, not a GS-local one.
    float aboveHomeAltitude;

    float groundSpeed;
    float airSpeed;
    float climbRate;

    float navigationAltitudeError;
    float navigationSpeedError;
    float navigationCrosstrackError;
    float navigationTargetBearing;

    Layout layout;      // The display layout.
    Style style;        // The AI style (tapes translucent or opague)

    // TODO: Use stylesheet colors?
    QColor redColor;
    QColor amberColor;
    QColor greenColor;

    qreal lineWidth;
    qreal fineLineWidth;

    qreal smallTextSize;
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

    static const int updateInterval = 50;
};

#endif // PRIMARYFLIGHTDISPLAY_H
