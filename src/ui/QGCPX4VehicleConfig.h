#ifndef QGCPX4VehicleConfig_H
#define QGCPX4VehicleConfig_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QGroupBox>
#include <QPushButton>
#include <QStringList>
#include <QGraphicsScene>

#include "QGCToolWidget.h"
#include "UASInterface.h"
#include "px4_configuration/QGCPX4AirframeConfig.h"

class UASParameterCommsMgr;
class QGCPX4SensorCalibration;
class PX4RCCalibration;

namespace Ui {
class QGCPX4VehicleConfig;
}

class QGCPX4VehicleConfig : public QWidget
{
    Q_OBJECT

public:
    explicit QGCPX4VehicleConfig(QWidget *parent = 0);
    ~QGCPX4VehicleConfig();

    enum RC_MODE {
        RC_MODE_1 = 1,
        RC_MODE_2 = 2,
        RC_MODE_3 = 3,
        RC_MODE_4 = 4,
        RC_MODE_NONE = 5
    };

public slots:
    void rcMenuButtonClicked();
    void sensorMenuButtonClicked();
    void tuningMenuButtonClicked();
    void flightModeMenuButtonClicked();
    void safetyConfigMenuButtonClicked();
    void advancedMenuButtonClicked();
    void airframeMenuButtonClicked();
    void firmwareMenuButtonClicked();

    /** Set the MAV currently being calibrated */
    void setActiveUAS(UASInterface* active);

protected slots:
    void menuButtonClicked();
    /** Parameter changed onboard */
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void updateStatus(const QString& str);
    void updateError(const QString& str);

protected:

    bool doneLoadingConfig;
    UASInterface* mav;                  ///< The current MAV
    QGCUASParamManagerInterface* paramMgr;       ///< params mgr for the mav
    QList<QGCToolWidget*> toolWidgets;  ///< Configurable widgets
    QMap<QString,QGCToolWidget*> toolWidgetsByName; ///<

    QMap<QString,QGCToolWidget*> paramToWidgetMap;                     ///< Holds the current active MAV's parameter widgets.
    QList<QWidget*> additionalTabs;                                   ///< Stores additional tabs loaded for this vehicle/autopilot configuration. Used for cleaning up.
    QMap<QString,QGCToolWidget*> libParamToWidgetMap;                  ///< Holds the library parameter widgets
    QMap<QString,QMap<QString,QGCToolWidget*> > systemTypeToParamMap;   ///< Holds all loaded MAV specific parameter widgets, for every MAV.
    QMap<QGCToolWidget*,QGroupBox*> toolToBoxMap;                       ///< Easy method of figuring out which QGroupBox is tied to which ToolWidget.
    QMap<QString,QString> paramTooltips;                                ///< Tooltips for the ? button next to a parameter.

    QGCPX4AirframeConfig* px4AirframeConfig;
    QPixmap planeBack;
    QPixmap planeSide;
    QGCPX4SensorCalibration* px4SensorCalibration;
    PX4RCCalibration* px4RCCalibration;
    QGraphicsScene scene;
    QPushButton* skipActionButton;

private:
    Ui::QGCPX4VehicleConfig *ui;
    QMap<QPushButton*,QWidget*> buttonToWidgetMap;
signals:
    void visibilityChanged(bool visible);
};

#endif // QGCPX4VehicleConfig_H
