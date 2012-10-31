#ifndef QGCHILXPLANECONFIGURATION_H
#define QGCHILXPLANECONFIGURATION_H

#include <QWidget>

#include "QGCHilLink.h"

namespace Ui {
class QGCHilXPlaneConfiguration;
}

class QGCHilXPlaneConfiguration : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCHilXPlaneConfiguration(QGCHilLink* link, QWidget *parent = 0);
    ~QGCHilXPlaneConfiguration();

public slots:
    /** @brief Start / stop simulation */
    void toggleSimulation(bool connect);

protected:
    QGCHilLink* link;
    
private:
    Ui::QGCHilXPlaneConfiguration *ui;
};

#endif // QGCHILXPLANECONFIGURATION_H
