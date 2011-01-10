#ifndef QGCPARAMSLIDER_H
#define QGCPARAMSLIDER_H

#include <QWidget>
#include <QAction>
#include <QtDesigner/QDesignerExportWidget>

#include "QGCToolWidgetItem.h"

namespace Ui {
    class QGCParamSlider;
}

class QGCParamSlider : public QGCToolWidgetItem
{
    Q_OBJECT

public:
    explicit QGCParamSlider(QWidget *parent = 0);
    ~QGCParamSlider();

public slots:
    void startEditMode();
    void endEditMode();
    /** @brief Send the parameter to the MAV */
    void sendParameter();
    void writeSettings(QSettings& settings);
    void readSettings(const QSettings& settings);

protected:
    QString parameterName;         ///< Key/Name of the parameter
    float parameterValue;          ///< Value of the parameter
    double parameterScalingFactor; ///< Factor to scale the parameter between slider and true value
    float parameterMin;
    float parameterMax;
    int component;                 ///< ID of the MAV component to address
    void changeEvent(QEvent *e);

private:
    Ui::QGCParamSlider *ui;
};

#endif // QGCPARAMSLIDER_H
