#ifndef QGCSENSORSETTINGSWIDGET_H
#define QGCSENSORSETTINGSWIDGET_H

#include <QWidget>

#include "UASInterface.h"

namespace Ui {
    class QGCSensorSettingsWidget;
}

class QGCSensorSettingsWidget : public QWidget {
    Q_OBJECT
public:
    QGCSensorSettingsWidget(UASInterface* uas, QWidget *parent = 0);
    ~QGCSensorSettingsWidget();

protected:
    UASInterface* mav;
    void changeEvent(QEvent *e);

private:
    Ui::QGCSensorSettingsWidget *ui;
};

#endif // QGCSENSORSETTINGSWIDGET_H
