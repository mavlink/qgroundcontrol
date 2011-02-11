#ifndef QGCSETTINGSWIDGET_H
#define QGCSETTINGSWIDGET_H

#include <QDialog>

namespace Ui {
    class QGCSettingsWidget;
}

class QGCSettingsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit QGCSettingsWidget(QWidget *parent = 0);
    ~QGCSettingsWidget();

private:
    Ui::QGCSettingsWidget *ui;
};

#endif // QGCSETTINGSWIDGET_H
