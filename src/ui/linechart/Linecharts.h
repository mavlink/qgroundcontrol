#ifndef LINECHARTS_H
#define LINECHARTS_H

#include <QStackedWidget>
#include <QMap>

#include "LinechartWidget.h"
#include "UASInterface.h"

class Linecharts : public QStackedWidget
{
Q_OBJECT
public:
    explicit Linecharts(QWidget *parent = 0);

signals:

public slots:
    /** @brief Set all plots active/inactive */
    void setActive(bool active);
    /** @brief Select plot for one system */
    void selectSystem(int systemid);
    /** @brief Add a new system to the list of plots */
    void addSystem(UASInterface* uas);

protected:
    QMap<int, LinechartWidget*> plots;
    bool active;
};

#endif // LINECHARTS_H
