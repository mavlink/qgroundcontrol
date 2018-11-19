#pragma once

#include <QWidget>

#include "QGCHilLink.h"

class QGCHilConfiguration;
namespace Ui {
class QGCHilXPlaneConfiguration;
}

class QGCHilXPlaneConfiguration : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCHilXPlaneConfiguration(QGCHilLink* link, QGCHilConfiguration *parent = 0);
    ~QGCHilXPlaneConfiguration();

public slots:
    /** @brief Start / stop simulation */
    void toggleSimulation(bool connect);
    /** @brief Set X-Plane version */
    void setVersion(int version);

protected:
    QGCHilLink* link;
    
private:
    Ui::QGCHilXPlaneConfiguration *ui;
};

