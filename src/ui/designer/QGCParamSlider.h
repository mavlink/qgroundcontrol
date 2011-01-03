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

protected:
    void changeEvent(QEvent *e);

private:
    Ui::QGCParamSlider *ui;
};

#endif // QGCPARAMSLIDER_H
